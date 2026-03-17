#include "ScreenTracedAmbientOcclusion.h"

#include <Render/Render.h>

void ScreenTracedAmbientOcclusionRenderer_s::Init()
{
	rl::ComputePipelineStateDesc PSODesc = {};
	PSODesc.Cs = rl::CreateComputeShader("Shared/Shaders/STAO/ScreenSpaceTracing.hlsl");
	PSODesc.DebugName = L"ScreenSpaceTracingCS";

	STAOPSO = rl::CreateComputePipelineState(PSODesc);

	Ready = STAOPSO != rl::ComputePipelineState_t::INVALID;
}

RenderGraphResourceHandle_t ScreenTracedAmbientOcclusionRenderer_s::GenerateSTAOTexture(RenderGraphBuilder_s& RGBuilder, RenderGraphResourceHandle_t SceneDepth)
{
	return RenderGraphResourceHandle_t::NONE;
}
