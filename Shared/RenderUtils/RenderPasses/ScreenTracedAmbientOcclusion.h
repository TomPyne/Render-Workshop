#pragma once

#include "RenderUtils/RenderGraph/RenderGraph.h"

#include <Render/RenderTypes.h>

struct ScreenTracedAmbientOcclusionRenderer_s
{
	rl::ComputePipelineStatePtr STAOPSO = {};

	bool Ready = false;

	void Init();

	RenderGraphResourceHandle_t GenerateSTAOTexture(RenderGraphBuilder_s& RGBuilder, RenderGraphResourceHandle_t SceneDepth);
};