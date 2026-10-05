#pragma once

#include <Render/RenderTypes.h>
#include <RenderUtils/RenderGraph/RenderGraph.h>

enum class DebugViewMode_e : uint32_t
{
	Lit = 0,
	Albedo,
	Normal,
	Roughness,
	Metallic,
	Specular,
	Emissive,
	AO,
	GlobalDistanceFieldSlice,
	GlobalDistanceField,
	Count
};

const char* GetDebugViewModeName(DebugViewMode_e Mode);

struct DebugViewInputs_s
{
	RenderGraphResourceHandle_t SceneColorMetallic;
	RenderGraphResourceHandle_t SceneNormalRoughness;
	RenderGraphResourceHandle_t SceneEmissiveSpecular;
	RenderGraphResourceHandle_t SceneAO;
	RenderGraphResourceHandle_t SceneDepth;

	// Temp
	uint32_t Frame;
	float Time;
	float Pad;
};

struct DebugViewRenderer_s
{
	void Init();
	void AddPass(RenderGraphBuilder_s& RGBuilder, DebugViewMode_e Mode, const DebugViewInputs_s& Inputs, RenderGraphResourceHandle_t Output);

private:
	rl::GraphicsPipelineStatePtr PSO = {};
};
