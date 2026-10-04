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
	Noise,
	GlobalDistanceFieldSlice,
	Count
};

const char* GetDebugViewModeName(DebugViewMode_e Mode);

struct DebugViewInputs_s
{
	RenderGraphResourceHandle_t SceneColorMetallic;
	RenderGraphResourceHandle_t SceneNormalRoughness;
	RenderGraphResourceHandle_t SceneEmissiveSpecular;
	RenderGraphResourceHandle_t SceneDepth;

	// Temp
	uint32_t BlueNoiseSRVIndex;
	uint32_t Frame;
	float Time;
	float Pad;
};

struct DebugViewRenderer_s
{
	void Init(rl::RootSignature_t InRootSignature, uint32_t InCBVRootSigSlot, uint32_t InSRVTableRootSigSlot);
	void AddPass(RenderGraphBuilder_s& RGBuilder, DebugViewMode_e Mode, const DebugViewInputs_s& Inputs, RenderGraphResourceHandle_t Output);

private:
	rl::RootSignaturePtr RootSignature = {};
	rl::GraphicsPipelineStatePtr PSO = {};

	uint32_t CBVRootSigSlot = 0;
	uint32_t SRVTableRootSigSlot = 0;
};
