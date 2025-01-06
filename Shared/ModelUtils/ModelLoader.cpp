#include "ModelLoader.h"

#include "Logging/Logging.h"
#include "Materials/Materials.h"
#include "Model.h"
#include "MeshProcessing/MeshProcessing.h"
#include "MeshProcessing/WaveFrontReader.h"
#include "Profiling/ScopeTimer.h"
#include "TextureUtils/TextureManager.h"

#include <SurfMath.h>

bool LoadModelFromWavefront(const wchar_t* WavefrontPath, ModelAsset_s& OutModel)
{
    WaveFrontReader_c Reader;
    {
        ScopeTimer_s ScopeTimer("Loaded Wavefront file");

        if (!Reader.Load(WavefrontPath))
        {
            return false;
        }
    }

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

    std::vector<float3> PositionReorder;
    PositionReorder.resize(OutModel.VertexCount);

    std::vector<uint8_t> IndexReorder;
    IndexReorder.resize(OutModel.IndexCount * sizeof(uint32_t));

    std::vector<uint32_t> FaceRemap;
    FaceRemap.resize(TriCount);

    std::vector<uint32_t> VertexRemap;
    VertexRemap.resize(OutModel.VertexCount);

    std::vector<uint32_t> DupedVerts;

    {
        ScopeTimer_s ScopeTimer("Clean and sort mesh by attributes");

        if (!ENSUREMSG(MeshProcessing::CleanMesh(reinterpret_cast<MeshProcessing::index_t*>(OutModel.Indices.data()), TriCount, OutModel.VertexCount, Attributes.data(), DupedVerts, true), "CleanMesh failed"))
            return false;

        if (!ENSUREMSG(MeshProcessing::AttributeSort(TriCount, Attributes.data(), FaceRemap.data()), "AttributeSort failed"))
            return false;

        if (!ENSUREMSG(MeshProcessing::ReorderIndices(reinterpret_cast<MeshProcessing::index_t*>(OutModel.Indices.data()), TriCount, FaceRemap.data(), reinterpret_cast<MeshProcessing::index_t*>(IndexReorder.data())), "ReorderIndices failed"))
            return false;

        std::swap(OutModel.Indices, IndexReorder);
    }

    {
        ScopeTimer_s ScopeTimer("Optimise mesh faces");

        if (!ENSUREMSG(MeshProcessing::OptimizeFacesLRU(reinterpret_cast<MeshProcessing::index_t*>(OutModel.Indices.data()), TriCount, FaceRemap.data()), "OptimizeFacesLRU failed"))
            return false;

        if (!ENSUREMSG(MeshProcessing::ReorderIndices(reinterpret_cast<MeshProcessing::index_t*>(OutModel.Indices.data()), TriCount, FaceRemap.data(), reinterpret_cast<MeshProcessing::index_t*>(IndexReorder.data())), "ReorderIndices failed"))
            return false;

        std::swap(OutModel.Indices, IndexReorder);
    }

    {
        ScopeTimer_s ScopeTimer("Optimise mesh vertices");

        if (!ENSUREMSG(MeshProcessing::OptimizeVertices(reinterpret_cast<MeshProcessing::index_t*>(OutModel.Indices.data()), TriCount, OutModel.VertexCount, VertexRemap.data()), "OptimizeVertices failed"))
            return false;
    }

    {
        ScopeTimer_s ScopeTimer("Finalise mesh buffers");

        if (!ENSUREMSG(MeshProcessing::FinalizeIndices(reinterpret_cast<MeshProcessing::index_t*>(OutModel.Indices.data()), TriCount, VertexRemap.data(), OutModel.VertexCount, reinterpret_cast<MeshProcessing::index_t*>(IndexReorder.data())), "FinalizeIndices failed"))
            return false;

        if (!ENSUREMSG(MeshProcessing::FinalizeVertices(OutModel.Positions.data(), sizeof(float3), OutModel.VertexCount, DupedVerts.data(), DupedVerts.size(), VertexRemap.data(), PositionReorder.data()), "FinalizeVertices(Position) failed"))
            return false;

        std::swap(OutModel.Indices, IndexReorder);
        std::swap(OutModel.Positions, PositionReorder);

        if (Reader.HasNormals)
        {
            std::vector<float3> NormalReorder;
            NormalReorder.resize(OutModel.VertexCount);

            if (!ENSUREMSG(MeshProcessing::FinalizeVertices(OutModel.Normals.data(), sizeof(float3), OutModel.VertexCount, DupedVerts.data(), DupedVerts.size(), VertexRemap.data(), NormalReorder.data()), "FinalizeVertices(Normal) failed"))
                return false;

            std::swap(OutModel.Normals, NormalReorder);
        }

        if (Reader.HasTexcoords)
        {
            std::vector<float2> UVReorder;
            UVReorder.resize(OutModel.VertexCount);

            if (!ENSUREMSG(MeshProcessing::FinalizeVertices(OutModel.Texcoords.data(), sizeof(float2), OutModel.VertexCount, DupedVerts.data(), DupedVerts.size(), VertexRemap.data(), UVReorder.data()), "FinalizeVertices(UV) failed"))
                return false;

            std::swap(OutModel.Texcoords, UVReorder);
        }
    }

    std::vector<MeshProcessing::Subset_s> Subsets;

    {
        ScopeTimer_s ScopeTimer("Compute mesh subsets");

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
        ScopeTimer_s ScopeTimer("Compute mesh normals");

        OutModel.Normals.resize(OutModel.VertexCount);

        if (!ENSUREMSG(MeshProcessing::ComputeNormals(reinterpret_cast<MeshProcessing::index_t*>(OutModel.Indices.data()), TriCount, OutModel.Positions.data(), OutModel.VertexCount, OutModel.Normals.data()), "ComputeNormals failed"))
            return false;

        OutModel.HasNormals = true;
    }

    if (Reader.HasNormals && Reader.HasTexcoords)
    {
        ScopeTimer_s ScopeTimer("Compute mesh tangents");

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
        ScopeTimer_s ScopeTimer("Meshletize mesh");

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

    OutModel.UniqueIndexCount = static_cast<uint32_t>(OutModel.UniqueVertexIndices.size());

    OutModel.PrimitiveIndexCount = static_cast<uint32_t>(PrimitiveIndices.size());
    OutModel.PrimitiveIndices.resize(PrimitiveIndices.size());

    for (uint32_t PrimIt = 0; PrimIt < OutModel.PrimitiveIndexCount; PrimIt++)
    {
        OutModel.PrimitiveIndices[PrimIt] = PrimitiveIndices[PrimIt].packed;
    }

    std::vector<std::wstring> MaterialPaths;

    for (uint32_t MaterialIt = 1; MaterialIt < Reader.Materials.size(); MaterialIt++)
    {
        MaterialAsset_s MaterialAsset;
        MaterialAsset.Albedo = Reader.Materials[MaterialIt].Diffuse;
        wcscpy_s(MaterialAsset.AlbedoTexturePath, Reader.Materials[MaterialIt].Texture.c_str());

        std::wstring MaterialPath = L"Assets/" + Reader.Materials[MaterialIt].Name + L".rmat";
        WriteMaterialAsset(MaterialPath, &MaterialAsset);

        MaterialPaths.push_back(MaterialPath);
    }

    OutModel.MeshCount = static_cast<uint32_t>(IndexSubsets.size());
    OutModel.Meshes.resize(IndexSubsets.size());
    for (uint32_t SubsetIt = 0; SubsetIt < IndexSubsets.size(); SubsetIt++)
    {
        MeshAsset_s& OutMesh = OutModel.Meshes[SubsetIt];
        const MeshProcessing::Subset_s& IndexSubset = IndexSubsets[SubsetIt];
        const MeshProcessing::Subset_s& MeshletSubset = MeshletSubsets[SubsetIt];
        OutMesh.IndexCount = IndexSubset.Count;
        OutMesh.IndexOffset = IndexSubset.Offset;

        OutMesh.MeshletCount = MeshletSubset.Count;
        OutMesh.Meshlets.resize(MeshletSubset.Count);

        if (SubsetIt < MaterialPaths.size())
        {
            wcscpy_s(OutMesh.MaterialPath, MaterialPaths[SubsetIt].c_str());
        }

        for (uint32_t MeshletIt = 0, MeshletOffset = MeshletSubset.Offset; MeshletIt < MeshletSubset.Count; MeshletIt++, MeshletOffset++)
        {
            MeshAsset_s::Meshlet_s& OutMeshlet = OutMesh.Meshlets[MeshletIt];
            const MeshProcessing::Meshlet_s& Meshlet = Meshlets[MeshletOffset];

            OutMeshlet.PrimCount = Meshlet.PrimCount;
            OutMeshlet.PrimOffset = Meshlet.PrimOffset;
            OutMeshlet.VertCount = Meshlet.VertCount;
            OutMeshlet.VertOffset = Meshlet.VertOffset;
        }
    }

#if 0

    std::vector<ModelMaterial_s> LoadedMaterials;
    LoadedMaterials.reserve(Reader.Attributes.size());

    for (const WaveFrontReader_c::Material_s& Mtl : Reader.Materials)
    {
        ModelMaterial_s NewMaterial = {};
        NewMaterial.Params.Albedo = Mtl.Diffuse;

        if (!Mtl.Texture.empty())
        {
            NewMaterial.AlbedoTexture = LoadTexture(Mtl.Texture, false);
            NewMaterial.Params.AlbedoTextureIndex = tpr::GetDescriptorIndex(NewMaterial.AlbedoTexture.SRV);
        }

        NewMaterial.MaterialBuffer = tpr::CreateConstantBuffer(&NewMaterial.Params);

        LoadedMaterials.push_back(NewMaterial);
    }

    CHECK(IndexSubsets.size() <= LoadedMaterials.size() - 1);

    for (uint32_t SubsetIt = 0; SubsetIt < IndexSubsets.size(); SubsetIt++)
    {
        SubMesh_s Mesh;
        Mesh.IndexOffset = IndexSubsets[SubsetIt].Offset;
        Mesh.IndexCount = IndexSubsets[SubsetIt].Count;
        Mesh.Material = LoadedMaterials[SubsetIt + 1];
        OutModel.Meshes.push_back(Mesh);
    }

    OutModel.MeshCount = static_cast<uint32_t>(OutModel.Meshes.size());

#endif

    return true;
}