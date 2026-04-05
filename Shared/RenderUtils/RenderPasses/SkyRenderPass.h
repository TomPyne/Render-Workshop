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
	uint32_t CBVRootSlot = 0u;

	bool Ready = false;

	void Init(uint32_t InCBVRootSlot, uint32_t InCBVSlot);

	void AddPass(RenderGraphBuilder_s& RGBuilder, RenderGraphResourceHandle_t SceneColorTarget, RenderGraphResourceHandle_t SceneDepth, const matrix& ViewProjection, const float3& CamPos, const float3& SunDirection);
};