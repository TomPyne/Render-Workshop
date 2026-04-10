#include "DisocclusionRenderPass.h"

#include <imgui.h>
#include <Render/Render.h>
#include <RenderUtils/GPUContext/GPUContext.h>

struct DisocclusionUniforms_s
{
	matrix InverseViewProjection;
	matrix PrevInverseViewProjection;

	uint32_t DepthTextureIndex;
	uint32_t PrevFrameDepthTextureIndex;
	uint32_t ConfidenceTextureIndex;
	uint32_t PrevConfidenceTextureIndex;

	uint32_t VelocityTextureIndex;
	float __Pad0[3];

	float2 ViewportSizeRcp;
	uint2 ViewportSize;
};

void DisocclusionRenderPass_s::Init(uint32_t InUAVTableSlot, uint32_t InSRVTableSlot, uint32_t InCBVRootSlot, uint32_t InCBVSlot)
{
	UAVTableSlot = InUAVTableSlot;
	SRVTableSlot = InSRVTableSlot;
	CBVRootSlot = InCBVRootSlot;

	rl::ShaderMacros Macros = { { "CBV_SLOT", InCBVSlot } };

	rl::ComputePipelineStateDesc PSODesc = {};
	PSODesc.Cs = rl::CreateComputeShader("Shared/Shaders/Denoising/GenerateDisocclusion.hlsl", Macros);
	PSODesc.DebugName = L"DisocclusionCS";

	DisocclusionPSO = rl::CreateComputePipelineState(PSODesc);

	Ready = DisocclusionPSO.IsValid();
}

RenderGraphResourceHandle_t DisocclusionRenderPass_s::AddPass(RenderGraphBuilder_s& RGBuilder, RenderGraphResourceHandle_t SceneDepth, RenderGraphResourceHandle_t PrevSceneDepth, RenderGraphResourceHandle_t SceneVelocity, const matrix& ViewProjection, const matrix& PrevViewProjection, uint2 ScreenDim)
{
	RenderGraphResourceHandle_t ConfidenceTexture = RGBuilder.CreateTexture(ScreenDim.x, ScreenDim.y, rl::RenderFormat::R8_UNORM, RenderGraphResourceAccessType_e::UAV | RenderGraphResourceAccessType_e::SRV, L"Confidence");
	RenderGraphResourceHandle_t ConfidenceHistoryTexture = RGBuilder.InjectTexture(SceneConfidenceHistory, L"PrevFrameConfidence");

	RenderGraphPass_s& DisocclusionPass = RGBuilder.AddPass(RenderGraphPassType_e::COMPUTE, L"Disocclusion Pass")
	.AccessResource(SceneDepth, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(PrevSceneDepth, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(ConfidenceTexture, RenderGraphResourceAccessType_e::UAV, RenderGraphLoadOp_e::DONT_CARE)
	.AccessResource(ConfidenceHistoryTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(SceneVelocity, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.SetExecuteCallback([=](RenderGraph_s& RG, GPUContext_s& Ctx)
	{
		DisocclusionUniforms_s DisocclusionUniforms;

		DisocclusionUniforms.InverseViewProjection = InverseMatrix(ViewProjection);
		DisocclusionUniforms.PrevInverseViewProjection = InverseMatrix(PrevViewProjection);

		DisocclusionUniforms.DepthTextureIndex = GetDescriptorIndex(RG.GetSRV(SceneDepth));
		DisocclusionUniforms.PrevFrameDepthTextureIndex = GetDescriptorIndex(RG.GetSRV(PrevSceneDepth));
		DisocclusionUniforms.ConfidenceTextureIndex = GetDescriptorIndex(RG.GetUAV(ConfidenceTexture));
		DisocclusionUniforms.PrevConfidenceTextureIndex = GetDescriptorIndex(RG.GetSRV(ConfidenceHistoryTexture));

		DisocclusionUniforms.VelocityTextureIndex = GetDescriptorIndex(RG.GetSRV(SceneVelocity));

		DisocclusionUniforms.ViewportSizeRcp = float2(1.0f / (float)ScreenDim.x, 1.0f / (float)ScreenDim.y);
		DisocclusionUniforms.ViewportSize = ScreenDim;

		rl::DynamicBuffer_t DisocclusionCBuf = rl::CreateDynamicConstantBuffer(&DisocclusionUniforms);

		Ctx.SetRootSignature();

		Ctx.SetComputeRootDescriptorTable(UAVTableSlot);
		Ctx.SetComputeRootDescriptorTable(SRVTableSlot);

		Ctx.SetPipelineState(DisocclusionPSO);

		Ctx.SetComputeRootCBV(CBVRootSlot, DisocclusionCBuf);

		Ctx.Dispatch(DivideRoundUp(ScreenDim.x, 8u), DivideRoundUp(ScreenDim.y, 8u), 1u);
	});

	RGBuilder.QueueTextureCopy(ConfidenceHistoryTexture, ConfidenceTexture);

	return ConfidenceTexture;
}

void DisocclusionRenderPass_s::Resize(uint32_t Width, uint32_t Height)
{
	SceneConfidenceHistory = CreateRenderGraphTexture(Width, Height, rl::RenderFormat::R8_UNORM, RenderGraphResourceAccessType_e::UAV | RenderGraphResourceAccessType_e::SRV, L"SceneConfidenceHistory");
}

void DisocclusionRenderPass_s::DrawImGuiMenu()
{

}
