#pragma once

#include "RenderUtils/RenderGraph/RenderGraph.h"

#include <Render/RenderTypes.h>
#include <SurfMath.h>

struct SkyRenderer_s
{
	rl::VertexBufferPtr SkySphereVertexBuffer = {};
	rl::IndexBufferPtr SkySphereIndexBuffer = {};
	rl::GraphicsPipelineStatePtr SkyPSO = {};
	uint32_t NumIndices = 0u;

	bool Ready = false;

	void Init();

	void AddPass(RenderGraphBuilder_s& RGBuilder, RenderGraphResourceHandle_t SceneColorTarget, const matrix& ViewProjection, uint32_t CBVSlot);
};