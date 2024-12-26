#pragma once

#include <Render/Render.h>

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

struct Mesh_s
{
	ModelVertexBuffer_s Position;
	ModelVertexBuffer_s Normal;
	ModelVertexBuffer_s Tangent;
	ModelVertexBuffer_s Texcoord;
	ModelVertexBuffer_s Color;
	ModelIndexBuffer_t Index;

	std::vector<tpr::InputElementDesc> InputDesc;

	uint32_t BoundBufferCount = 0u;
	tpr::VertexBuffer_t BindBuffers[5] = { tpr::VertexBuffer_t::INVALID };
	uint32_t BindStrides[5] = { 0 };
	uint32_t BindOffsets[5] = { 0 };
};

struct Model_s
{
	std::vector<Mesh_s> Meshes;
};

bool LoadModelFromOBJ(const char* OBJPath, Model_s& OutModel);
bool LoadModelFromWavefront(const wchar_t* WavefrontPath, Model_s& OutModel);