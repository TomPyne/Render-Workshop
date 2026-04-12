#pragma once

#include "RenderUtils/RenderGraph/RenderGraph.h"

struct BloomRenderPass_s
{
	rl::ComputePipelineStatePtr BloomDownsamplePSO = {};
	rl::ComputePipelineStatePtr BloomUpsamplePSO = {};
	rl::ComputePipelineStatePtr BloomApplyPSO = {};

	uint32_t UAVTableSlot = 0;
	uint32_t SRVTableSlot = 0;
	uint32_t CBVRootSlot = 0;

	uint2 ScreenDim;

	std::vector<uint2> DownsampledDims;

	bool Ready = false;
	bool MenuOpen = false;

	void Init(uint32_t InUAVTableSlot, uint32_t InSRVTableSlot, uint32_t InCBVRootSlot, uint32_t InCBVSlot);
	void AddPass(RenderGraphBuilder_s& RGBuilder, RenderGraphResourceHandle_t SceneColor);
	void Resize(uint32_t Width, uint32_t Height);
	void DrawImGuiMenu();

private:
	RenderGraphResourceHandle_t DownsamplePass(RenderGraphBuilder_s& RGBuilder, RenderGraphResourceHandle_t Input, uint2 Dim, float Threshold);
	void UpsamplePass(RenderGraphBuilder_s& RGBuilder, RenderGraphResourceHandle_t Input, RenderGraphResourceHandle_t Output, uint2 Dim);
};