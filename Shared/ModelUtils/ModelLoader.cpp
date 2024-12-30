#include "ModelLoader.h"

#include "Logging/Logging.h"
#include "Model.h"
#include "MeshProcessing/MeshProcessing.h"
#include "MeshProcessing/WaveFrontReader.h"
#include "Profiling/ScopeTimer.h"
#include "TextureUtils/TextureManager.h"

#include <SurfMath.h>

bool LoadModelFromWavefront(const wchar_t* WavefrontPath, SubModel_s& OutModel)
{
    WaveFrontReader_c Reader;
    {
        ScopeTimer_s ScopeTimer("Loaded Wavefront file");

        if (!Reader.Load(WavefrontPath))
        {
            return false;
        }
    }

    std::vector<float3> Positions;
    std::vector<float3> Normals;
    std::vector<float2> UVs;
    std::vector<uint8_t> Indices;
    std::vector<uint32_t> Attributes;

    Positions.resize(Reader.Vertices.size());

    if (Reader.HasNormals)
    {
        Normals.resize(Reader.Vertices.size());
    }

    if (Reader.HasTexcoords)
    {
        UVs.resize(Reader.Vertices.size());
    }

    for (uint32_t VertIt = 0; VertIt < Reader.Vertices.size(); VertIt++)
    {
        Positions[VertIt] = Reader.Vertices[VertIt].Position;

        if (Reader.HasNormals)
        {
            Normals[VertIt] = Reader.Vertices[VertIt].Normal;
        }

        if (Reader.HasTexcoords)
        {
            UVs[VertIt] = Reader.Vertices[VertIt].Texcoord;
        }
    }

    Indices.resize(Reader.Indices.size() * sizeof(uint32_t));
    std::memcpy(Indices.data(), Reader.Indices.data(), Reader.Indices.size() * sizeof(uint32_t));

    Attributes = Reader.Attributes;

    const uint32_t IndexCount = static_cast<uint32_t>(Reader.Indices.size());
    const uint32_t VertexCount = static_cast<uint32_t>(Positions.size());
    const uint32_t TriCount = IndexCount / 3;

    std::vector<float3> PositionReorder;
    PositionReorder.resize(VertexCount);

    std::vector<uint8_t> IndexReorder;
    IndexReorder.resize(IndexCount * sizeof(uint32_t));

    std::vector<uint32_t> FaceRemap;
    FaceRemap.resize(TriCount);

    std::vector<uint32_t> VertexRemap;
    VertexRemap.resize(VertexCount);

    std::vector<uint32_t> DupedVerts;

    {
        ScopeTimer_s ScopeTimer("Clean and sort mesh by attributes");

        if (!ENSUREMSG(MeshProcessing::CleanMesh(reinterpret_cast<MeshProcessing::index_t*>(Indices.data()), TriCount, VertexCount, Attributes.data(), DupedVerts, true), "CleanMesh failed"))
            return false;

        if (!ENSUREMSG(MeshProcessing::AttributeSort(TriCount, Attributes.data(), FaceRemap.data()), "AttributeSort failed"))
            return false;

        if (!ENSUREMSG(MeshProcessing::ReorderIndices(reinterpret_cast<MeshProcessing::index_t*>(Indices.data()), TriCount, FaceRemap.data(), reinterpret_cast<MeshProcessing::index_t*>(IndexReorder.data())), "ReorderIndices failed"))
            return false;

        std::swap(Indices, IndexReorder);
    }

    {
        ScopeTimer_s ScopeTimer("Optimise mesh faces");

        if (!ENSUREMSG(MeshProcessing::OptimizeFacesLRU(reinterpret_cast<MeshProcessing::index_t*>(Indices.data()), TriCount, FaceRemap.data()), "OptimizeFacesLRU failed"))
            return false;

        if (!ENSUREMSG(MeshProcessing::ReorderIndices(reinterpret_cast<MeshProcessing::index_t*>(Indices.data()), TriCount, FaceRemap.data(), reinterpret_cast<MeshProcessing::index_t*>(IndexReorder.data())), "ReorderIndices failed"))
            return false;

        std::swap(Indices, IndexReorder);
    }


    {
        ScopeTimer_s ScopeTimer("Optimise mesh vertices");

        if (!ENSUREMSG(MeshProcessing::OptimizeVertices(reinterpret_cast<MeshProcessing::index_t*>(Indices.data()), TriCount, VertexCount, VertexRemap.data()), "OptimizeVertices failed"))
            return false;
    }


    {
        ScopeTimer_s ScopeTimer("Finalise mesh buffers");

        if (!ENSUREMSG(MeshProcessing::FinalizeIndices(reinterpret_cast<MeshProcessing::index_t*>(Indices.data()), TriCount, VertexRemap.data(), VertexCount, reinterpret_cast<MeshProcessing::index_t*>(IndexReorder.data())), "FinalizeIndices failed"))
            return false;

        if (!ENSUREMSG(MeshProcessing::FinalizeVertices(Positions.data(), sizeof(float3), VertexCount, DupedVerts.data(), DupedVerts.size(), VertexRemap.data(), PositionReorder.data()), "FinalizeVertices(Position) failed"))
            return false;

        std::swap(Indices, IndexReorder);
        std::swap(Positions, PositionReorder);

        if (Reader.HasNormals)
        {
            std::vector<float3> NormalReorder;
            NormalReorder.resize(VertexCount);

            if (!ENSUREMSG(MeshProcessing::FinalizeVertices(Normals.data(), sizeof(float3), VertexCount, DupedVerts.data(), DupedVerts.size(), VertexRemap.data(), NormalReorder.data()), "FinalizeVertices(Normal) failed"))
                return false;

            std::swap(Normals, NormalReorder);
        }

        if (Reader.HasTexcoords)
        {
            std::vector<float2> UVReorder;
            UVReorder.resize(VertexCount);

            if (!ENSUREMSG(MeshProcessing::FinalizeVertices(UVs.data(), sizeof(float2), VertexCount, DupedVerts.data(), DupedVerts.size(), VertexRemap.data(), UVReorder.data()), "FinalizeVertices(UV) failed"))
                return false;

            std::swap(UVs, UVReorder);
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

        Normals.resize(VertexCount);

        if (!ENSUREMSG(MeshProcessing::ComputeNormals(reinterpret_cast<MeshProcessing::index_t*>(Indices.data()), TriCount, Positions.data(), VertexCount, Normals.data()), "ComputeNormals failed"))
            return false;
    }

    std::vector<float4> Tangents;
    std::vector<float3> Bitangents;
    if (Reader.HasNormals && Reader.HasTexcoords)
    {
        ScopeTimer_s ScopeTimer("Compute mesh tangents");

        Tangents.resize(VertexCount);
        Bitangents.resize(VertexCount);

        if (!ENSUREMSG(MeshProcessing::ComputeTangents(reinterpret_cast<MeshProcessing::index_t*>(Indices.data()), TriCount, Positions.data(), Normals.data(), UVs.data(), VertexCount, Tangents.data(), Bitangents.data()), "ComputeTangents failed"))
            return false;
    }

    std::vector<MeshProcessing::Meshlet_s> Meshlets;
    std::vector<uint8_t> UniqueVertexIndices;
    std::vector<MeshProcessing::PackedTriangle_s> PrimitiveIndices;
    std::vector<MeshProcessing::Subset_s> MeshletSubsets;

    {
        ScopeTimer_s ScopeTimer("Meshletize mesh");

        constexpr uint32_t MeshletMaxVerts = 64;
        constexpr uint32_t MeshletMaxPrims = 126;

        if (!ENSUREMSG(MeshProcessing::ComputeMeshlets(
            MeshletMaxVerts, MeshletMaxPrims,
            reinterpret_cast<MeshProcessing::index_t*>(Indices.data()), IndexCount,
            IndexSubsets.data(), static_cast<uint32_t>(IndexSubsets.size()),
            Positions.data(), static_cast<uint32_t>(Positions.size()),
            MeshletSubsets,
            Meshlets,
            UniqueVertexIndices,
            PrimitiveIndices
        ), "ComputeMeshlets failed"))
            return false;
    }

    OutModel.Position.Init(Positions.data(), Positions.size());
    OutModel.Normal.Init(Normals.data(), Normals.size());
    OutModel.Texcoord.Init(UVs.data(), UVs.size());
    OutModel.Index.Init(reinterpret_cast<uint32_t*>(Indices.data()), Indices.size() / sizeof(uint32_t));

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

    return true;
}