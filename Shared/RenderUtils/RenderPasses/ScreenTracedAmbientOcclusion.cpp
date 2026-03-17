#include "ScreenTracedAmbientOcclusion.h"

#include <Render/Render.h>

struct STAOUniforms_s
{
	matrix Projection;
	matrix InverseProjection;
	matrix View;

	uint32_t DepthTextureIndex;
	uint32_t NormalTextureIndex;
	uint32_t OutputTextureIndex;
	float __Pad;

	float2 ViewportSizeRcp;
	uint2 ViewportSize;
};

void ScreenTracedAmbientOcclusionRenderer_s::Init(uint32_t InUAVTableSlot, uint32_t InSRVTableSlot, uint32_t InCBVSlot)
{
	UAVTableSlot = InUAVTableSlot;
	SRVTableSlot = InSRVTableSlot;
	CBVSlot = InCBVSlot;

	rl::ComputePipelineStateDesc PSODesc = {};
	PSODesc.Cs = rl::CreateComputeShader("Shared/Shaders/STAO/ScreenSpaceTracing.hlsl");
	PSODesc.DebugName = L"ScreenSpaceTracingCS";

	STAOPSO = rl::CreateComputePipelineState(PSODesc);

	Ready = STAOPSO != rl::ComputePipelineState_t::INVALID;
}

RenderGraphResourceHandle_t ScreenTracedAmbientOcclusionRenderer_s::GenerateSTAOTexture(
	RenderGraphBuilder_s& RGBuilder,
	RenderGraphResourceHandle_t SceneDepth,
	RenderGraphResourceHandle_t SceneNormal,
	const matrix& Projection,
	const matrix& View,
	uint2 ScreenDim)
{
	RenderGraphResourceHandle_t STAOTexture = RGBuilder.CreateTexture(ScreenDim.x, ScreenDim.y, rl::RenderFormat::R8_UNORM, RenderGraphResourceAccessType_e::UAV | RenderGraphResourceAccessType_e::SRV, L"STAOTexture");

	RGBuilder.AddPass(RenderGraphPassType_e::COMPUTE, L"STAO Pass")
	.AccessResource(SceneDepth, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(SceneNormal, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(STAOTexture, RenderGraphResourceAccessType_e::UAV, RenderGraphLoadOp_e::DONT_CARE)
	.SetExecuteCallback([=](RenderGraph_s& RG, rl::CommandList* CL)
	{
		STAOUniforms_s Uniforms;
		Uniforms.Projection = Projection;
		Uniforms.InverseProjection = InverseMatrix(Projection);
		Uniforms.View = View;
		Uniforms.DepthTextureIndex = RG.GetSRVIndex(SceneDepth);
		Uniforms.NormalTextureIndex = RG.GetSRVIndex(SceneNormal);
		Uniforms.OutputTextureIndex = RG.GetUAVIndex(STAOTexture);
		Uniforms.ViewportSizeRcp = float2(1.0f / (float)ScreenDim.x, 1.0f / (float)ScreenDim.y);
		Uniforms.ViewportSize = ScreenDim;

		CL->SetRootSignature();
		CL->SetComputeRootDescriptorTable(UAVTableSlot);
		CL->SetComputeRootDescriptorTable(SRVTableSlot);
		CL->SetComputeRootCBV(CBVSlot, rl::CreateDynamicConstantBuffer(&Uniforms));

		CL->SetPipelineState(STAOPSO);

		CL->Dispatch(DivideRoundUp(ScreenDim.x, 8u), DivideRoundUp(ScreenDim.y, 8u), 1u);
	});

	return STAOTexture;
}
