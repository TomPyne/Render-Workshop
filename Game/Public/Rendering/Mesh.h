#pragma once

#include <Render/RenderTypes.h>
#include <SurfMath.h>

#include <cstdint>
#include <vector>

struct Surface_s
{
	std::shared_ptr<class MaterialShaderInstance_c> Material = nullptr;
	uint32_t IndexOffset = 0;
	uint32_t IndexCount = 0;

	AABB Bounds = {};
};

struct MeshUniformData_s
{
	uint32_t PositionBufferIndex;
	uint32_t NormalBufferIndex;
	uint32_t TangentBufferIndex;
	uint32_t Texcoord0BufferIndex;
};

struct ObjectUniforms_s
{
	matrix ModelMatrix;
	matrix NormalMatrix;
	float DeterminantSign;
	float __Pad[3];
};

// Returns true when the transform mirrors, so the caller can select the front-face-culling PSO.
bool MakeObjectUniforms(const matrix& WorldMatrix, ObjectUniforms_s& OutUniforms);

struct Mesh_s
{
	bool Ready = false;

	std::vector<Surface_s> Surfaces;

	rl::StructuredBufferPtr PositionBuffer = {};
	rl::StructuredBufferPtr NormalBuffer = {};
	rl::StructuredBufferPtr TangentBuffer = {};
	rl::StructuredBufferPtr Texcoord0Buffer = {};
	rl::ShaderResourceViewPtr PositionBufferSRV = {};
	rl::ShaderResourceViewPtr NormalBufferSRV = {};
	rl::ShaderResourceViewPtr TangentBufferSRV = {};
	rl::ShaderResourceViewPtr Texcoord0BufferSRV = {};

	rl::IndexBufferPtr IndexBuffer = {};

	rl::ConstantBuffer_t MeshUniforms = {};

	std::vector<float3> Vertices;
	std::vector<uint32_t> Indices;

	AABB Bounds = {};

	void Render(struct SpatialRenderingCollector_s& Collector, rl::DynamicBuffer_t DynamicUniforms, bool Mirrored) const;
};