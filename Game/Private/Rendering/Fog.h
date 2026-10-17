#pragma once

#include "RenderUtils/RenderGraph/RenderGraph.h"

#include <SurfMath.h>

struct HeightFogInfo_s;

struct HeightFogInputs_s
{
	RenderGraphResourceHandle_t SceneColor = RenderGraphResourceHandle_t::NONE;
	RenderGraphResourceHandle_t SceneDepth = RenderGraphResourceHandle_t::NONE;
	// Optional, without it the inscatter falls back to AmbientLuminance
	RenderGraphResourceHandle_t SkyPrefilter = RenderGraphResourceHandle_t::NONE;
	float SkyIntensity = 0.0f;
	float SkyPrefilterMaxMip = 0.0f;

	matrix InvViewProjection;
	float3 CameraPosition;
	float3 SunDirection;
	float3 SunIrradiance;
	float3 AmbientLuminance;

	uint2 ScreenSize;
};

struct FogRenderer_s
{
	rl::ComputePipelineStatePtr HeightFogPSO = {};

	bool Ready = false;

	void Init();
	// Fogs SceneColor in place, sky pixels are treated as lying at Settings.SkyDistance
	void AddPass(RenderGraphBuilder_s& RGBuilder, const HeightFogInfo_s& Settings, const HeightFogInputs_s& Inputs);
};
