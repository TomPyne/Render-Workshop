#pragma once

#include "RenderUtils/RenderGraph/RenderGraph.h"

struct DisocclusionRenderPass_s
{
	rl::ComputePipelineStatePtr DisocclusionPSO = {};
	uint32_t UAVTableSlot = 0;
	uint32_t SRVTableSlot = 0;
	uint32_t CBVRootSlot = 0;

	RenderGraphTexturePtr_t SceneConfidenceHistory = {};

	bool Ready = false;
	bool MenuOpen = false;

	void Init(uint32_t InUAVTableSlot, uint32_t InSRVTableSlot, uint32_t InCBVRootSlot, uint32_t InCBVSlot);
	RenderGraphResourceHandle_t AddPass(RenderGraphBuilder_s& RGBuilder, RenderGraphResourceHandle_t SceneDepth, RenderGraphResourceHandle_t PrevSceneDepth, RenderGraphResourceHandle_t SceneVelocity, const matrix& ViewProjection, const matrix& PrevViewProjection, uint2 ScreenDim);
	void Resize(uint32_t Width, uint32_t Height);
	void DrawImGuiMenu();
};