#include "ModelLoader.h"

#include "Logging/Logging.h"
#include "Model.h"
#include "MeshProcessing/MeshProcessing.h"
#include "MeshProcessing/WaveFrontReader.h"
#include "Profiling/ScopeTimer.h"
#include "TextureUtils/TextureManager.h"

#include <SurfMath.h>

struct Subset_s
{
    uint32_t Offset;
    uint32_t Count;
};

bool LoadModelFromWavefront(const wchar_t* WavefrontPath, Model_s& OutModel)
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

    std::vector<MeshProcessing::Subset_t> Subsets;

    {
        ScopeTimer_s ScopeTimer("Compute mesh subsets");

        Subsets = MeshProcessing::ComputeSubsets(Attributes.data(), Attributes.size());
    }
    

    //std::vector<Subset_s> IndexSubsets;
    //IndexSubsets.resize(Subsets.size());
    //for (uint32_t SubsetIt = 0; SubsetIt < Subsets.size(); SubsetIt++)
    //{
    //    IndexSubsets[SubsetIt].Offset = static_cast<uint32_t>(Subsets[SubsetIt].first) * 3;
    //    IndexSubsets[SubsetIt].Count = static_cast<uint32_t>(Subsets[SubsetIt].second) * 3;
    //}

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
    std::vector<MeshProcessing::MeshletTriangle_s> PrimitiveIndices;
    std::vector<MeshProcessing::Subset_t> MeshletSubsets;
    MeshletSubsets.resize(Subsets.size());

    {
        ScopeTimer_s ScopeTimer("Meshletize mesh");

        if (!ENSUREMSG(MeshProcessing::ComputeMeshlets(
            reinterpret_cast<MeshProcessing::index_t*>(Indices.data()), TriCount,
            Positions.data(), VertexCount,
            Subsets.data(), static_cast<uint32_t>(Subsets.size()),
            Meshlets, UniqueVertexIndices, PrimitiveIndices,
            MeshletSubsets.data()
        ), "ComputeMeshlets failed"))
            return false;
    }

    return true;

    // Multiple meshes share one vertex buffer, break up the index buffers by attribute
    ModelVertexBuffer_s SharedPositionBuffer = {};
    {
        std::vector<float3> Positions;
        Positions.resize(Reader.Vertices.size());

        for (size_t Vert = 0u; Vert < Reader.Vertices.size(); Vert++)
        {
            Positions[Vert] = Reader.Vertices[Vert].Position;
        }

        SharedPositionBuffer.Init(Positions.data(), Positions.size());
    }

    ModelVertexBuffer_s SharedTexcoordBuffer = {};
    {
        std::vector<float2> Texcoords;

        if (Reader.HasTexcoords)
        {
            Texcoords.resize(Reader.Vertices.size());

            for (size_t Vert = 0u; Vert < Reader.Vertices.size(); Vert++)
            {
                Texcoords[Vert] = Reader.Vertices[Vert].Texcoord;
                Texcoords[Vert].y = 1.0f - Texcoords[Vert].y;
            }
        }
        else
        {
            Texcoords.resize(Reader.Vertices.size(), float2(0, 0));
        }

        SharedTexcoordBuffer.Init(Texcoords.data(), Texcoords.size());
    }

    ModelVertexBuffer_s SharedNormalBuffer = {};
    ModelVertexBuffer_s SharedTangentBuffer = {};
    {
        std::vector<float3> Normals;
        Normals.resize(Reader.Vertices.size());
        if (Reader.HasNormals)
        {
            for (size_t Vert = 0u; Vert < Reader.Vertices.size(); Vert++)
            {
                Normals[Vert] = Reader.Vertices[Vert].Normal;
            }
        }
        else // Calculate normals
        {
            for (uint32_t IndexIt = 0u, TriIt = 0u; IndexIt < Reader.Indices.size(); IndexIt += 3, TriIt++)
            {
                uint32_t I0 = Reader.Indices[IndexIt];
                uint32_t I1 = Reader.Indices[IndexIt + 1];
                uint32_t I2 = Reader.Indices[IndexIt + 2];

                float3 P0 = Reader.Vertices[I0].Position;
                float3 P1 = Reader.Vertices[I1].Position;
                float3 P2 = Reader.Vertices[I2].Position;

                const float3 V10 = P1 - P0;
                const float3 V20 = P2 - P0;
                Normals[I0] = Normals[I1] = Normals[I2] = Normalize(Cross(V10, V20));
            }
        }

        SharedNormalBuffer.Init(Normals.data(), Normals.size());
    }

    std::vector<std::vector<uint32_t>> AttributeIndices;

    AttributeIndices.resize(Reader.Materials.size());

    for (size_t AttrIt = 0, IndexIt = 0; AttrIt < Reader.Attributes.size(); AttrIt++, IndexIt += 3)
    {
        uint32_t Attr = Reader.Attributes[AttrIt];

        assert(Attr < AttributeIndices.size());

        AttributeIndices[Attr].push_back(Reader.Indices[IndexIt]);
        AttributeIndices[Attr].push_back(Reader.Indices[IndexIt + 1]);
        AttributeIndices[Attr].push_back(Reader.Indices[IndexIt + 2]);
    }

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

    for (uint32_t AttrIt = 0u; AttrIt < AttributeIndices.size(); AttrIt++)
    {
        const std::vector<uint32_t>& Indices = AttributeIndices[AttrIt];

        if (Indices.empty())
            continue;

        Mesh_s Mesh;
        Mesh.Index.Init(Indices.data(), Indices.size());

        Mesh.Position = SharedPositionBuffer;
        Mesh.Texcoord = SharedTexcoordBuffer;
        Mesh.Normal = SharedNormalBuffer;

        Mesh.InputDesc[0] = tpr::InputElementDesc("POSITION", 0u, tpr::RenderFormat::R32G32B32_FLOAT, 0u, 0u, tpr::InputClassification::PER_VERTEX, 0u);
        Mesh.InputDesc[1] = tpr::InputElementDesc("TEXCOORD", 0u, tpr::RenderFormat::R32G32_FLOAT, 1u, 0u, tpr::InputClassification::PER_VERTEX, 0u);
        Mesh.InputDesc[2] = tpr::InputElementDesc("NORMAL", 0u, tpr::RenderFormat::R32G32B32_FLOAT, 2u, 0u, tpr::InputClassification::PER_VERTEX, 0u);

        Mesh.BindBuffers[0] = SharedPositionBuffer.Buffer;
        Mesh.BindBuffers[1] = SharedTexcoordBuffer.Buffer;
        Mesh.BindBuffers[2] = SharedNormalBuffer.Buffer;

        Mesh.BindStrides[0] = sizeof(float3);
        Mesh.BindStrides[1] = sizeof(float2);
        Mesh.BindStrides[2] = sizeof(float3);

        Mesh.BindOffsets[0] = 0u;
        Mesh.BindOffsets[1] = 0u;
        Mesh.BindOffsets[2] = 0u;

        Mesh.BoundBufferCount = 3u;

        Mesh.Material = LoadedMaterials[AttrIt];

        OutModel.Meshes.push_back(Mesh);
    }

    return true;
}