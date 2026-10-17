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
	// Exposure is the tonemapper's, so firefly suppression judges brightness as it will be displayed
	void AddPass(RenderGraphBuilder_s& RGBuilder, RenderGraphResourceHandle_t SceneColor, uint2 ScreenSize, float Exposure);

private:
	RenderGraphResourceHandle_t DownsamplePass(RenderGraphBuilder_s& RGBuilder, RenderGraphResourceHandle_t Input, uint2 Dim, float Threshold, float Exposure);
	void UpsamplePass(RenderGraphBuilder_s& RGBuilder, RenderGraphResourceHandle_t Input, RenderGraphResourceHandle_t Output, uint2 Dim);
};