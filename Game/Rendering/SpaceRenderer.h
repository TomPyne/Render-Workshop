#pragma once

#include <Render/RenderTypes.h>

#include <unordered_map>
#include <vector>

struct SpatialRenderingBatch_s
{
	rl::DynamicBuffer_t DynamicUniforms;
	rl::ConstantBuffer_t MeshUniforms;
	rl::ConstantBuffer_t MaterialUniforms;
	rl::IndexBuffer_t IndexBuffer;
	rl::RenderFormat IndexBufferFormat;
	uint32_t IndexCount;
	rl::GraphicsPipelineState_t PSO;
};

struct SpatialRenderingMeshPass_s
{
	std::vector<SpatialRenderingBatch_s> Batches;

	SpatialRenderingBatch_s& AddBatch()
	{
		Batches.resize(Batches.size() + 1);
		return Batches.back();
	}
};

enum class SpatialShader_t : uint32_t
{
	INVALID = 0,
};

enum class SpatialShaderPass_t : uint32_t
{
	INVALID = 0,
};

struct SpatialRenderingCollector_s
{
	SpatialRenderingMeshPass_s MainPass;
};

struct SpaceRendererScreenInfo_s
{
	uint32_t Width;
	uint32_t Height;
};

class SpaceRenderer_c
{
public:
	void RenderSpace(const SpaceRendererScreenInfo_s& Screen, class Space_c* Space, rl::CommandListSubmissionGroup& clGroup);
};