#pragma once

#include "RenderUtils/RenderGraph/RenderGraph.h"

#include <Render/RenderTypes.h>
#include <SurfMath.h>

struct ScreenTracedAmbientOcclusionRenderer_s
{
	rl::ComputePipelineStatePtr STAOPSO = {};

	uint32_t UAVTableSlot = 0;
	uint32_t SRVTableSlot = 0;
	uint32_t CBVSlot = 0;

	bool Ready = false;

	void Init(uint32_t InUAVTableSlot, uint32_t InSRVTableSlot, uint32_t InCBVSlot);

	RenderGraphResourceHandle_t GenerateSTAOTexture(
		RenderGraphBuilder_s& RGBuilder, 
		RenderGraphResourceHandle_t SceneDepth,
		RenderGraphResourceHandle_t SceneNormal,
		const matrix& Projection,
		const matrix& View,
		uint2 ScreenDim);
};