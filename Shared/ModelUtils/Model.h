#pragma once

#include "TextureUtils/TextureManager.h"
#include <Render/Render.h>
#include <SurfMath.h>
#include <vector>

struct ModelVertexBuffer_s
{
	tpr::VertexBufferPtr Buffer = tpr::VertexBuffer_t::INVALID;
	uint32_t Stride = 0u;
	uint32_t Offset = 0u;

	template<typename T>
	void Init(const T* const Data, size_t ElemCount)
	{
		Buffer = tpr::CreateVertexBufferFromArray(Data, ElemCount);
		Stride = static_cast<uint32_t>(sizeof(T));
		Offset = 0u;
	}
};

struct ModelIndexBuffer_t
{
	tpr::IndexBufferPtr Buffer = tpr::IndexBuffer_t::INVALID;
	tpr::RenderFormat Format = tpr::RenderFormat::UNKNOWN;
	uint32_t Offset = 0;
	uint32_t Count = 0;

	void Init(const uint32_t* const Data, size_t ElemCount)
	{
		Buffer = tpr::CreateIndexBufferFromArray(Data, ElemCount);
		Format = tpr::RenderFormat::R32_UINT;
		Offset = 0u;
		Count = static_cast<uint32_t>(ElemCount);
	}

	void Init(const uint16_t* const Data, size_t ElemCount)
	{
		Buffer = tpr::CreateIndexBufferFromArray(Data, ElemCount);
		Format = tpr::RenderFormat::R16_UINT;
		Offset = 0u;
		Count = static_cast<uint32_t>(ElemCount);
	}
};

struct ModelMaterial_s
{
	struct Params_s
	{
		float3 Albedo;
		uint32_t AlbedoTextureIndex;
	} Params;

	TextureAsset_s AlbedoTexture;

	tpr::ConstantBufferPtr MaterialBuffer = {};
};

struct Mesh_s
{
	ModelVertexBuffer_s Position;
	ModelVertexBuffer_s Normal;
	ModelVertexBuffer_s Tangent;
	ModelVertexBuffer_s Texcoord;
	ModelVertexBuffer_s Color;
	ModelIndexBuffer_t Index;

	ModelMaterial_s Material;

	tpr::InputElementDesc InputDesc[5];

	uint32_t BoundBufferCount = 0u;
	tpr::VertexBuffer_t BindBuffers[5] = { tpr::VertexBuffer_t::INVALID };
	uint32_t BindStrides[5] = { 0 };
	uint32_t BindOffsets[5] = { 0 };
};

struct SubMeshlet_s
{
	uint32_t IndexOffset;
	uint32_t IndexCount;
};

struct SubMesh_s
{
	uint32_t IndexOffset;
	uint32_t IndexCount;

	std::vector<SubMeshlet_s> Meshlets;

	ModelMaterial_s Material;
};

struct SubModel_s
{
	ModelVertexBuffer_s Position;
	ModelVertexBuffer_s Normal;
	ModelVertexBuffer_s Texcoord;
	ModelIndexBuffer_t Index;

	std::vector<SubMesh_s> Meshes;
};

struct Model_s
{
	std::vector<Mesh_s> Meshes;
};
