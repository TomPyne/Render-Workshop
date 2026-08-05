#pragma once

#include <Render/RenderTypes.h>

#include <cstdint>
#include <vector>

struct Surface_s
{
	class BasicMaterial_c* Material = nullptr;
	uint32_t IndexOffset = 0;
	uint32_t IndexCount = 0;
};

struct MeshUniformData_s
{
	uint32_t PositionBufferIndex;
	float __pad[3];
};

struct Mesh_s
{
	bool Ready = false;

	std::vector<Surface_s> Surfaces;

	rl::StructuredBufferPtr PositionBuffer = {};
	rl::ShaderResourceViewPtr PositionBufferSRV = {};

	rl::IndexBufferPtr IndexBuffer = {};

	rl::ConstantBuffer_t MeshUniforms = {};

	void Render(struct SpatialRenderingCollector_s& Collector, rl::DynamicBuffer_t DynamicUniforms) const;
};