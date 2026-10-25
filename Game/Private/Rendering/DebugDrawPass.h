#pragma once

#include "Rendering/DebugDraw.h"

#include <RenderUtils/RenderGraph/RenderGraph.h>

#include <SurfMath.h>

struct DebugDrawRenderer_s
{
	void Init();
	// Unlit overlay of the collector's debug vertices onto the display ready Target, which must match SceneDepth's size
	void AddPass(RenderGraphBuilder_s& RGBuilder, const struct SpatialRenderingCollector_s& Collector, RenderGraphResourceHandle_t Target, RenderGraphResourceHandle_t SceneDepth,
		FrameBufferAlloc_s ViewUniforms, uint2 ScreenSize);

private:
	rl::GraphicsPipelineStatePtr PSOs[static_cast<size_t>(DebugPrimitive_e::Count)][static_cast<size_t>(DebugDepthMode_e::Count)] = {};
};
