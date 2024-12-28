#include "ModelLoader.h"

#include "Model.h"
#include "TextureUtils/TextureManager.h"

#include <MeshProcessing/WaveFrontReader.h>

#include <SurfMath.h>

bool LoadModelFromWavefront(const wchar_t* WavefrontPath, Model_s& OutModel)
{
    WaveFrontReader_c Reader;
    if (!Reader.Load(WavefrontPath))
    {
        return false;
    }

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