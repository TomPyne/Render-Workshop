#include "HPModelPipe.h"

#include "HPModel.h"
#include "MeshProcessing.h"
#include "WaveFrontReader.h"

#include <FileUtils/FileStream.h>
#include <FileUtils/PathUtils.h>
#include <Logging/Logging.h>
#include <Profiling/ScopeTimer.h>

bool LoadModelFromWavefront(const std::wstring& SourceDir, const std::wstring& OutputDir, const wchar_t* WavefrontPath, HPModel_s& OutModel)
{
    WaveFrontReader_c Reader;
    {
        ScopeTimer_s ScopeTimer(L"Loaded Wavefront file " + std::wstring(WavefrontPath));

        if (!Reader.Load(WavefrontPath))
        {
            return false;
        }
    }

    std::wstring MtlLibCookedPath;
    if(!Reader.MtlFile.empty())
    {
        std::wstring AbsolutePath = MakePathAbsolute(Reader.MtlFile);
        std::wstring ContentRelativePath = MakePathRelativeTo(AbsolutePath, SourceDir);

        HPAssetArgs_s MtlLibArgs;
        MtlLibArgs.AssetType = L"WfMtlLib";
        MtlLibArgs.Args.push_back(L"-src");
        MtlLibArgs.Args.push_back(ContentRelativePath);
        PushCookCommand(MtlLibArgs);

        MtlLibCookedPath = GetPackagePathForAssetFromArgs(MtlLibArgs);
    }

    OutModel.MaterialLibPath = MtlLibCookedPath;

    std::vector<uint32_t> Attributes;

    OutModel.Positions.resize(Reader.Vertices.size());

    if (Reader.HasNormals)
    {
        OutModel.HasNormals = true;
        OutModel.Normals.resize(Reader.Vertices.size());
    }

    if (Reader.HasTexcoords)
    {
        OutModel.HasTexcoords = true;
        OutModel.Texcoords.resize(Reader.Vertices.size());
    }

    for (uint32_t VertIt = 0; VertIt < Reader.Vertices.size(); VertIt++)
    {
        OutModel.Positions[VertIt] = Reader.Vertices[VertIt].Position;

        if (Reader.HasNormals)
        {
            OutModel.Normals[VertIt] = Reader.Vertices[VertIt].Normal;
        }

        if (Reader.HasTexcoords)
        {
            OutModel.Texcoords[VertIt] = Reader.Vertices[VertIt].Texcoord;
        }
    }

    OutModel.Indices.resize(Reader.Indices.size() * sizeof(uint32_t));
    std::memcpy(OutModel.Indices.data(), Reader.Indices.data(), Reader.Indices.size() * sizeof(uint32_t));

    Attributes = Reader.Attributes;

    OutModel.IndexCount = static_cast<uint32_t>(Reader.Indices.size());
    OutModel.VertexCount = static_cast<uint32_t>(OutModel.Positions.size());
    OutModel.IndexFormat = tpr::RenderFormat::R32_UINT;
    const uint32_t TriCount = OutModel.IndexCount / 3;

    std::vector<uint8_t> IndexReorder;
    IndexReorder.resize(OutModel.IndexCount * sizeof(uint32_t));

    std::vector<uint32_t> FaceRemap;
    FaceRemap.resize(TriCount);

    {
        ScopeTimer_s ScopeTimer(L"Clean and sort mesh by attributes " + std::wstring(WavefrontPath));

        std::vector<uint32_t> DupedVerts;

        if (!ENSUREMSG(MeshProcessing::CleanMesh(reinterpret_cast<MeshProcessing::index_t*>(OutModel.Indices.data()), TriCount, OutModel.VertexCount, Attributes.data(), DupedVerts, true), "CleanMesh failed"))
            return false;

        if (!DupedVerts.empty())
        {
            std::vector<float3> PositionReorder;
            PositionReorder.resize(OutModel.VertexCount + DupedVerts.size());

            if (!ENSUREMSG(MeshProcessing::FinalizeVertices(OutModel.Positions.data(), sizeof(float3), OutModel.VertexCount, DupedVerts.data(), DupedVerts.size(), nullptr, PositionReorder.data()), "FinalizeVertices(Position) failed"))
                return false;

            std::swap(OutModel.Positions, PositionReorder);

            if (Reader.HasNormals)
            {
                std::vector<float3> NormalReorder;
                NormalReorder.resize(OutModel.VertexCount + DupedVerts.size());

                if (!ENSUREMSG(MeshProcessing::FinalizeVertices(OutModel.Normals.data(), sizeof(float3), OutModel.VertexCount, DupedVerts.data(), DupedVerts.size(), nullptr, NormalReorder.data()), "FinalizeVertices(Normal) failed"))
                    return false;

                std::swap(OutModel.Normals, NormalReorder);
            }

            if (Reader.HasTexcoords)
            {
                std::vector<float2> UVReorder;
                UVReorder.resize(OutModel.VertexCount + DupedVerts.size());

                if (!ENSUREMSG(MeshProcessing::FinalizeVertices(OutModel.Texcoords.data(), sizeof(float2), OutModel.VertexCount, DupedVerts.data(), DupedVerts.size(), nullptr, UVReorder.data()), "FinalizeVertices(UV) failed"))
                    return false;

                std::swap(OutModel.Texcoords, UVReorder);
            }

            OutModel.VertexCount += DupedVerts.size();
        }

        if (!ENSUREMSG(MeshProcessing::AttributeSort(TriCount, Attributes.data(), FaceRemap.data()), "AttributeSort failed"))
            return false;

        if (!ENSUREMSG(MeshProcessing::ReorderIndices(reinterpret_cast<MeshProcessing::index_t*>(OutModel.Indices.data()), TriCount, FaceRemap.data(), reinterpret_cast<MeshProcessing::index_t*>(IndexReorder.data())), "ReorderIndices failed"))
            return false;

        std::swap(OutModel.Indices, IndexReorder);
    }

    {
        ScopeTimer_s ScopeTimer(L"Optimise mesh faces " + std::wstring(WavefrontPath));

        if (!ENSUREMSG(MeshProcessing::OptimizeFacesLRU(reinterpret_cast<MeshProcessing::index_t*>(OutModel.Indices.data()), TriCount, FaceRemap.data()), "OptimizeFacesLRU failed"))
            return false;

        if (!ENSUREMSG(MeshProcessing::ReorderIndices(reinterpret_cast<MeshProcessing::index_t*>(OutModel.Indices.data()), TriCount, FaceRemap.data(), reinterpret_cast<MeshProcessing::index_t*>(IndexReorder.data())), "ReorderIndices failed"))
            return false;

        std::swap(OutModel.Indices, IndexReorder);
    }

    std::vector<uint32_t> VertexRemap;
    VertexRemap.resize(OutModel.VertexCount);

    {
        ScopeTimer_s ScopeTimer(L"Optimise mesh vertices " + std::wstring(WavefrontPath));

        if (!ENSUREMSG(MeshProcessing::OptimizeVertices(reinterpret_cast<MeshProcessing::index_t*>(OutModel.Indices.data()), TriCount, OutModel.VertexCount, VertexRemap.data()), "OptimizeVertices failed"))
            return false;
    }

    {
        ScopeTimer_s ScopeTimer(L"Finalise mesh buffers " + std::wstring(WavefrontPath));

        if (!ENSUREMSG(MeshProcessing::FinalizeIndices(reinterpret_cast<MeshProcessing::index_t*>(OutModel.Indices.data()), TriCount, VertexRemap.data(), OutModel.VertexCount, reinterpret_cast<MeshProcessing::index_t*>(IndexReorder.data())), "FinalizeIndices failed"))
            return false;

        std::vector<float3> PositionReorder;
        PositionReorder.resize(OutModel.VertexCount);

        if (!ENSUREMSG(MeshProcessing::FinalizeVertices(OutModel.Positions.data(), sizeof(float3), OutModel.VertexCount, nullptr, 0u, VertexRemap.data(), PositionReorder.data()), "FinalizeVertices(Position) failed"))
            return false;

        std::swap(OutModel.Indices, IndexReorder);
        std::swap(OutModel.Positions, PositionReorder);

        if (Reader.HasNormals)
        {
            std::vector<float3> NormalReorder;
            NormalReorder.resize(OutModel.VertexCount);

            if (!ENSUREMSG(MeshProcessing::FinalizeVertices(OutModel.Normals.data(), sizeof(float3), OutModel.VertexCount, nullptr, 0u, VertexRemap.data(), NormalReorder.data()), "FinalizeVertices(Normal) failed"))
                return false;

            std::swap(OutModel.Normals, NormalReorder);
        }

        if (Reader.HasTexcoords)
        {
            std::vector<float2> UVReorder;
            UVReorder.resize(OutModel.VertexCount);

            if (!ENSUREMSG(MeshProcessing::FinalizeVertices(OutModel.Texcoords.data(), sizeof(float2), OutModel.VertexCount, nullptr, 0u, VertexRemap.data(), UVReorder.data()), "FinalizeVertices(UV) failed"))
                return false;

            std::swap(OutModel.Texcoords, UVReorder);
        }
    }

    std::vector<MeshProcessing::Subset_s> Subsets;

    {
        ScopeTimer_s ScopeTimer(L"Compute mesh subsets " + std::wstring(WavefrontPath));

        Subsets = MeshProcessing::ComputeSubsets(Attributes.data(), Attributes.size());
    }

    std::vector<MeshProcessing::Subset_s> IndexSubsets;
    IndexSubsets.resize(Subsets.size());
    for (uint32_t SubsetIt = 0; SubsetIt < Subsets.size(); SubsetIt++)
    {
        IndexSubsets[SubsetIt].Offset = static_cast<uint32_t>(Subsets[SubsetIt].Offset) * 3;
        IndexSubsets[SubsetIt].Count = static_cast<uint32_t>(Subsets[SubsetIt].Count) * 3;
    }

    if (!Reader.HasNormals)
    {
        ScopeTimer_s ScopeTimer(L"Compute mesh normals " + std::wstring(WavefrontPath));

        OutModel.Normals.resize(OutModel.VertexCount);

        if (!ENSUREMSG(MeshProcessing::ComputeNormals(reinterpret_cast<MeshProcessing::index_t*>(OutModel.Indices.data()), TriCount, OutModel.Positions.data(), OutModel.VertexCount, OutModel.Normals.data()), "ComputeNormals failed"))
            return false;

        OutModel.HasNormals = true;
    }

    if (Reader.HasNormals && Reader.HasTexcoords)
    {
        ScopeTimer_s ScopeTimer(L"Compute mesh tangents " + std::wstring(WavefrontPath));

        OutModel.Tangents.resize(OutModel.VertexCount);
        OutModel.Bitangents.resize(OutModel.VertexCount);

        if (!ENSUREMSG(MeshProcessing::ComputeTangents(reinterpret_cast<MeshProcessing::index_t*>(OutModel.Indices.data()), TriCount, OutModel.Positions.data(), OutModel.Normals.data(), OutModel.Texcoords.data(), OutModel.VertexCount, OutModel.Tangents.data(), OutModel.Bitangents.data()), "ComputeTangents failed"))
            return false;

        OutModel.HasTangents = true;
        OutModel.HasBitangents = true;
    }

    std::vector<MeshProcessing::Meshlet_s> Meshlets;
    std::vector<MeshProcessing::PackedTriangle_s> PrimitiveIndices;
    std::vector<MeshProcessing::Subset_s> MeshletSubsets;

    {
        ScopeTimer_s ScopeTimer(L"Meshletize mesh " + std::wstring(WavefrontPath));

        constexpr uint32_t MeshletMaxVerts = 64;
        constexpr uint32_t MeshletMaxPrims = 126;

        if (!ENSUREMSG(MeshProcessing::ComputeMeshlets(
            MeshletMaxVerts, MeshletMaxPrims,
            reinterpret_cast<MeshProcessing::index_t*>(OutModel.Indices.data()), OutModel.IndexCount,
            IndexSubsets.data(), static_cast<uint32_t>(IndexSubsets.size()),
            OutModel.Positions.data(), static_cast<uint32_t>(OutModel.Positions.size()),
            MeshletSubsets,
            Meshlets,
            OutModel.UniqueVertexIndices,
            PrimitiveIndices
        ), "ComputeMeshlets failed"))
            return false;
    }

    OutModel.PrimitiveIndices.resize(PrimitiveIndices.size());

    memcpy(OutModel.PrimitiveIndices.data(), PrimitiveIndices.data(), sizeof(uint32_t) * PrimitiveIndices.size());

    bool UsingMaterials = !Reader.Materials.empty();
    if (UsingMaterials)
    {
        UsingMaterials = Reader.Materials.size() == IndexSubsets.size() + 1;
    }

    OutModel.Meshes.resize(IndexSubsets.size());
    for (uint32_t SubsetIt = 0; SubsetIt < IndexSubsets.size(); SubsetIt++)
    {
        HPModel_s::Mesh_s& OutMesh = OutModel.Meshes[SubsetIt];
        const MeshProcessing::Subset_s& IndexSubset = IndexSubsets[SubsetIt];
        const MeshProcessing::Subset_s& MeshletSubset = MeshletSubsets[SubsetIt];
        OutMesh.IndexCount = IndexSubset.Count;
        OutMesh.IndexOffset = IndexSubset.Offset;

        OutMesh.MeshletCount = MeshletSubset.Count;
        OutMesh.MeshletOffset = MeshletSubset.Offset;

        if (UsingMaterials)
        {
            OutMesh.LibMaterialName = Reader.Materials[SubsetIt + 1].Name;
        }
    }

    OutModel.Meshlets.resize(Meshlets.size());

    for (uint32_t MeshletIt = 0; MeshletIt < Meshlets.size(); MeshletIt++)
    {
        HPModel_s::Meshlet_s& OutMeshlet = OutModel.Meshlets[MeshletIt];
        const MeshProcessing::Meshlet_s& Meshlet = Meshlets[MeshletIt];

        OutMeshlet.PrimCount = Meshlet.PrimCount;
        OutMeshlet.PrimOffset = Meshlet.PrimOffset;
        OutMeshlet.VertCount = Meshlet.VertCount;
        OutMeshlet.VertOffset = Meshlet.VertOffset;
    }

    return true;
}

static std::wstring GenerateOutputPath(const std::wstring& OutputDir, const std::wstring& AssetPath)
{
    return OutputDir + L"/" + ReplacePathExtension(AssetPath, L"hp_mdl");
}

void HPModelPipe_c::Cook(const std::wstring& SourceDir, const std::wstring& OutputDir, const HPArgs_t& Args)
{
    std::wstring AssetPath;
    if (!ParseArgs(Args, L"-src", AssetPath))
    {
        LOGERROR("[HPModelPipe] No source path provided for asset");
        return;
    }

    std::wstring AbsSrcPath = SourceDir + L"/" + AssetPath;

    HPModel_s ProcessedModel;

    if (HasPathExtension(AssetPath, L".obj"))
    {
        if (!LoadModelFromWavefront( SourceDir, OutputDir, AbsSrcPath.c_str(), ProcessedModel))
        {
            LOGERROR("[HPModelPipe] Failed to process model");
            return;
        }
    }
    else
    {
        LOGERROR("[HPModelPipe] Unsupported file extension for source asset [%S]", AssetPath.c_str());
        return;
    }

    std::wstring AssetOutputPath = GenerateOutputPath(OutputDir, AssetPath);
   
    CreateDirectories(AssetOutputPath);

    if (!ProcessedModel.Serialize(AssetOutputPath, FileStreamMode_e::WRITE))
    {
        LOGERROR("[HPModelPipe] Failed to write asset [%S]", AssetOutputPath.c_str());
        return;
    }

    LOGINFO("[HPModelPipe] Cooked model [%S]", AssetOutputPath.c_str());
}

std::wstring HPModelPipe_c::GetCookedAssetPath(const std::wstring& OutputDir, const HPArgs_t& Args) const
{
    std::wstring AssetPath;
    if (!ParseArgs(Args, L"-src", AssetPath))
    {
        LOGERROR("[HPModelPipe] No source path provided for asset");
        return {};
    }

    return GenerateOutputPath(OutputDir, AssetPath);
}

std::wstring HPModelPipe_c::GetPackageAssetPath(const HPArgs_t& Args) const
{
    std::wstring AssetPath;
    if (!ParseArgs(Args, L"-src", AssetPath))
    {
        LOGERROR("[HPModelPipe] No source path provided for asset");
        return {};
    }
    return ReplacePathExtension(AssetPath, L"hp_mdl");
}
