#pragma once

#include "RenderUtils/RenderGraph/RenderGraph.h"

#include <Render/RenderTypes.h>

enum class TonemapMode_e : uint32_t
{
	None = 0,
	ACES = 1,
	Filmic = 2,
	Count
};

struct TonemapRenderer_s
{
	uint32_t SRVTableRootSigSlot = 0;
	uint32_t CBVRootSigSlot = 0;

	rl::RootSignaturePtr RootSignaure = {};

	bool Ready = false;

	void Init(rl::RootSignature_t InRootSignaure, uint32_t InCBVRootSlot, uint32_t InCBVSlot, uint32_t InSRVTableRootSigSlot);
	void AddPass(RenderGraphBuilder_s& RGBuilder, TonemapMode_e Mode, RenderGraphResourceHandle_t Input, RenderGraphResourceHandle_t Output);

	rl::GraphicsPipelineStatePtr NoTonemapPSO = {};
	rl::GraphicsPipelineStatePtr ACESTonemapPSO = {};
	rl::GraphicsPipelineStatePtr FilmicTonemapPSO = {};

private:
	void FullScreenPassVSPS(RenderGraph_s& RG, GPUContext_s& Ctx, RenderGraphResourceHandle_t Target, rl::GraphicsPipelineState_t PSO, rl::DynamicBuffer_t UniformBuffer);
};