#pragma once

#include <HPWfMtlLib.h>
#include <Render/RenderTypes.h>

#include <cstdint>
#include <vector>

namespace RTDDrawConstantSlots_e
{
	enum
	{
		INDEX_OFFSET = 0,
		MESHLET_OFFSET = 0,
		COUNT,
	};
}

struct RTDTexture_s
{
	rl::TexturePtr Texture = {};
	rl::ShaderResourceViewPtr SRV = {};

	bool Init(const struct HPTexture_s& Asset);
};

struct RTDMaterialParams_s
{
	uint32_t AlbedoTextureIndex = 0;
	uint32_t NormalTextureIndex = 0;
	uint32_t RoughnessMetallicTextureIndex = 0;
	float __Pad;
};

struct RTDMaterial_s
{
	RTDMaterialParams_s Params;

	std::shared_ptr<RTDTexture_s> AlbedoTexture = nullptr;
	std::shared_ptr<RTDTexture_s> NormalTexture = nullptr;
	std::shared_ptr<RTDTexture_s> RoughnessMetallicTexture = nullptr;

	rl::ConstantBuffer_t MaterialConstantBuffer = {};

	bool Init(const struct HPWfMtlLib_s::Material_s& Asset);
};

struct RTDBuffer_s
{
	rl::StructuredBufferPtr StructuredBuffer = {};
	rl::ShaderResourceViewPtr BufferSRV = {};

	void InitInternal(const void* const Data, size_t Count, size_t ElemSize);

	template<typename T>
	void Init(const T* const Data, size_t Count)
	{
		if (Count)
		{
			InitInternal(Data, Count, sizeof(T));
		}
	}
};

struct RTDMeshConstants_s
{
	// Vertex data
	uint32_t PositionBufSRVIndex = 0;
	uint32_t NormalBufSRVIndex = 0;
	uint32_t TangentBufSRVIndex = 0;
	uint32_t BitangentBufSRVIndex = 0;
	uint32_t TexcoordBufSRVIndex = 0;
	uint32_t IndexBufSRVIndex = 0;
	// Mesh shader data
	uint32_t MeshletBufSRVIndex = 0;
	uint32_t UniqueVertexIndexBufSRVIndex = 0;
	uint32_t PrimitiveIndexBufSRVIndex = 0;

	float __pad[3];
};

struct RTDMesh_s
{
	uint32_t IndexOffset;
	uint32_t IndexCount;

	uint32_t MeshletOffset;
	uint32_t MeshletCount;

	rl::RaytracingGeometryPtr RaytracingGeometry = {};

	std::shared_ptr<RTDMaterial_s> Material = nullptr;
};

struct RTDModel_s
{
	RTDBuffer_s PositionBuffer;
	RTDBuffer_s NormalBuffer;
	RTDBuffer_s TangentBuffer;
	RTDBuffer_s BitangentBuffer;
	RTDBuffer_s TexcoordBuffer;
	RTDBuffer_s IndexBuffer;

	RTDBuffer_s MeshletBuffer;
	RTDBuffer_s UniqueVertexIndexBuffer;
	RTDBuffer_s PrimitiveIndexBuffer;

	rl::ConstantBufferPtr ModelConstantBuffer;

	std::vector<RTDMesh_s> Meshes;

	bool Init(const struct HPModel_s* Asset);
};