#pragma once

#include "RenderUtils/RenderGraph/RenderGraph.h"

#include <SurfMath.h>
#include <vector>

struct BloomRenderer_s
{
	rl::ComputePipelineStatePtr BloomDownsamplePSO = {};
	rl::ComputePipelineStatePtr BloomUpsamplePSO = {};
	rl::ComputePipelineStatePtr BloomApplyPSO = {};

	bool Ready = false;

	void Init();
	void AddPass(RenderGraphBuilder_s& RGBuilder, RenderGraphResourceHandle_t SceneColor, uint2 ScreenSize);

private:
	RenderGraphResourceHandle_t DownsamplePass(RenderGraphBuilder_s& RGBuilder, RenderGraphResourceHandle_t Input, uint2 Dim, float Threshold);
	void UpsamplePass(RenderGraphBuilder_s& RGBuilder, RenderGraphResourceHandle_t Input, RenderGraphResourceHandle_t Output, uint2 Dim);
};