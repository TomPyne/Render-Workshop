#pragma once

#include "RenderUtils/RenderGraph/RenderGraph.h"

#include <Render/RenderTypes.h>
#include <SurfMath.h>

struct ScreenTracedReflectionRenderer_s
{
	rl::ComputePipelineStatePtr STRPSO = {};

	uint32_t UAVTableSlot = 0;
	uint32_t SRVTableSlot = 0;
	uint32_t CBVRootSlot = 0;

	bool Ready = false;

	bool MenuOpen = false;

	// Parameters
	float Thickness = 0.5f;
	float MaxDistance = 100.0f;
	int MaxSteps = 30;
	float Stride = 1.0f;
	float Jitter = 0.0f;

	void Init(uint32_t InUAVTableSlot, uint32_t InSRVTableSlot, uint32_t InCBVRootSlot, uint32_t InCBVSlot);

	RenderGraphResourceHandle_t GenerateSTRTexture(
		RenderGraphBuilder_s& RGBuilder,
		RenderGraphResourceHandle_t SceneDepth,
		RenderGraphResourceHandle_t SceneColor,
		RenderGraphResourceHandle_t SceneNormal,
		const matrix& Projection,
		const matrix& PixelProjection,
		const matrix& View,
		uint2 ScreenDim,
		float NearPlane);

	void DrawImGuiMenu();
};