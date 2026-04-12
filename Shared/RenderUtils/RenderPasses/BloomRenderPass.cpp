#include "BloomRenderPass.h"
#include <Render/Render.h>
#include <RenderUtils/GPUContext/GPUContext.h>

void BloomRenderPass_s::Init(uint32_t InUAVTableSlot, uint32_t InSRVTableSlot, uint32_t InCBVRootSlot, uint32_t InCBVSlot)
{
	UAVTableSlot = InUAVTableSlot;
	SRVTableSlot = InSRVTableSlot;
	CBVRootSlot = InCBVRootSlot;

	rl::ShaderMacros Macros = { { "CBV_SLOT", InCBVSlot } };

	rl::ComputePipelineStateDesc PSODesc = {};

	PSODesc.Cs = rl::CreateComputeShader("Shared/Shaders/Bloom/BloomDownsample.hlsl", Macros);
	PSODesc.DebugName = L"BloomDownsampleCS";
	BloomDownsamplePSO = rl::CreateComputePipelineState(PSODesc);

	PSODesc.Cs = rl::CreateComputeShader("Shared/Shaders/Bloom/BloomUpsample.hlsl", Macros);
	PSODesc.DebugName = L"BloomUpsampleCS";
	BloomUpsamplePSO = rl::CreateComputePipelineState(PSODesc);

	PSODesc.Cs = rl::CreateComputeShader("Shared/Shaders/Bloom/BloomApply.hlsl", Macros);
	PSODesc.DebugName = L"BloomApplyCS";
	BloomApplyPSO = rl::CreateComputePipelineState(PSODesc);

	Ready = BloomDownsamplePSO.IsValid() && BloomUpsamplePSO.IsValid() && BloomApplyPSO.IsValid();
}

RenderGraphResourceHandle_t BloomRenderPass_s::DownsamplePass(RenderGraphBuilder_s& RGBuilder, RenderGraphResourceHandle_t Input, uint2 Dim, float Threshold)
{
	const std::wstring ResourceName = L"BloomDownsampled_" + std::to_wstring(Dim.x) + L"x" + std::to_wstring(Dim.y);
	RenderGraphResourceHandle_t Output = RGBuilder.CreateTexture(Dim.x, Dim.y, rl::RenderFormat::R16G16B16A16_FLOAT, RenderGraphResourceAccessType_e::UAV | RenderGraphResourceAccessType_e::SRV, ResourceName.c_str());

	RGBuilder.AddPass(RenderGraphPassType_e::COMPUTE, ResourceName.c_str())
	.AccessResource(Input, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(Output, RenderGraphResourceAccessType_e::UAV, RenderGraphLoadOp_e::DONT_CARE)
	.SetExecuteCallback([=](RenderGraph_s& RG, GPUContext_s& Ctx)
	{
		const uint2 SrcDim = RG.GetTextureDimensions(Input);
		struct
		{
			uint32_t InputTextureIndex;
			uint32_t OutputTextureIndex;
			uint2 Dim;

			float2 DimRcp;
			float2 SrcDimRcp;

			float Threshold;
			float __Pad0[3];
		} DownsampleUniforms;

		DownsampleUniforms.InputTextureIndex = RG.GetSRVIndex(Input);
		DownsampleUniforms.OutputTextureIndex = RG.GetUAVIndex(Output);
		DownsampleUniforms.Dim = Dim;
		DownsampleUniforms.DimRcp = float2(1.0f / static_cast<float>(Dim.x), 1.0f / static_cast<float>(Dim.y));
		DownsampleUniforms.SrcDimRcp = float2(1.0f / static_cast<float>(SrcDim.x), 1.0f / static_cast<float>(SrcDim.y));
		DownsampleUniforms.Threshold = Threshold;

		Ctx.SetRootSignature();

		Ctx.SetComputeRootDescriptorTable(UAVTableSlot);
		Ctx.SetComputeRootDescriptorTable(SRVTableSlot);

		Ctx.SetPipelineState(BloomDownsamplePSO);

		Ctx.SetComputeRootCBV(CBVRootSlot, rl::CreateDynamicConstantBuffer(&DownsampleUniforms));

		Ctx.Dispatch(DivideRoundUp(Dim.x, 8u), DivideRoundUp(Dim.y, 8u), 1u);
	});

	return Output;
}

void BloomRenderPass_s::UpsamplePass(RenderGraphBuilder_s& RGBuilder, RenderGraphResourceHandle_t Input, RenderGraphResourceHandle_t Output, uint2 Dim)
{
	const std::wstring ResourceName = L"BloomUpsampled_" + std::to_wstring(Dim.x) + L"x" + std::to_wstring(Dim.y);

	RGBuilder.AddPass(RenderGraphPassType_e::COMPUTE, ResourceName.c_str())
	.AccessResource(Input, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(Output, RenderGraphResourceAccessType_e::UAV, RenderGraphLoadOp_e::DONT_CARE)
	.SetExecuteCallback([=](RenderGraph_s& RG, GPUContext_s& Ctx)
	{
		struct
		{
			uint32_t InputTextureIndex;
			uint32_t OutputTextureIndex;
			uint2 Dim;

			float2 DimRcp;
			float FilterRadius;
			float __Pad0[1];
		} UpsampleUniforms;

		UpsampleUniforms.InputTextureIndex = RG.GetSRVIndex(Input);
		UpsampleUniforms.OutputTextureIndex = RG.GetUAVIndex(Output);
		UpsampleUniforms.Dim = Dim;
		UpsampleUniforms.DimRcp = float2(1.0f / static_cast<float>(Dim.x), 1.0f / static_cast<float>(Dim.y));
		UpsampleUniforms.FilterRadius = 0.01f;

		Ctx.SetRootSignature();
		Ctx.SetComputeRootDescriptorTable(UAVTableSlot);
		Ctx.SetComputeRootDescriptorTable(SRVTableSlot);
		Ctx.SetPipelineState(BloomUpsamplePSO);
		Ctx.SetComputeRootCBV(CBVRootSlot, rl::CreateDynamicConstantBuffer(&UpsampleUniforms));
		Ctx.Dispatch(DivideRoundUp(Dim.x, 8u), DivideRoundUp(Dim.y, 8u), 1u);
	});
}

void BloomRenderPass_s::AddPass(RenderGraphBuilder_s& RGBuilder, RenderGraphResourceHandle_t SceneColor)
{
	std::vector<RenderGraphResourceHandle_t> DownsampledTextures;
	DownsampledTextures.reserve(DownsampledDims.size());
	for (const uint2 DownSampleDim : DownsampledDims)
	{
		const bool FirstPass = DownsampledTextures.empty();
		DownsampledTextures.push_back(DownsamplePass(RGBuilder, FirstPass ? SceneColor : DownsampledTextures.back(), DownSampleDim, FirstPass ? 1.0f : -1.0f));
	}

	for(int32_t i = static_cast<int32_t>(DownsampledDims.size()) - 1; i >= 1; --i)
	{
		const uint2 UpsampleDim = DownsampledDims[i - 1];
		const RenderGraphResourceHandle_t Input = DownsampledTextures[i];
		const RenderGraphResourceHandle_t Output = DownsampledTextures[i - 1];
		UpsamplePass(RGBuilder, Input, Output, UpsampleDim);
	}

	RGBuilder.AddPass(RenderGraphPassType_e::COMPUTE, L"Bloom Apply")
	.AccessResource(DownsampledTextures[0], RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(SceneColor, RenderGraphResourceAccessType_e::UAV, RenderGraphLoadOp_e::LOAD)
	.SetExecuteCallback([=](RenderGraph_s& RG, GPUContext_s& Ctx)
	{
		struct
		{
			uint32_t BloomTextureIndex;
			uint32_t SceneColorTextureIndex;
			uint2 Dim;

			float2 SrcDimRcp;
			float BloomIntensity;
			float __Pad0[1];
		} BloomApplyUniforms;

		BloomApplyUniforms.BloomTextureIndex = RG.GetSRVIndex(DownsampledTextures[0]);
		BloomApplyUniforms.SceneColorTextureIndex = RG.GetUAVIndex(SceneColor);
		BloomApplyUniforms.Dim = ScreenDim;
		BloomApplyUniforms.SrcDimRcp = float2(1.0f / static_cast<float>(ScreenDim.x), 1.0f / static_cast<float>(ScreenDim.y));
		BloomApplyUniforms.BloomIntensity = 0.1f;

		Ctx.SetRootSignature();
		Ctx.SetComputeRootDescriptorTable(UAVTableSlot);
		Ctx.SetComputeRootDescriptorTable(SRVTableSlot);
		Ctx.SetPipelineState(BloomApplyPSO);
		Ctx.SetComputeRootCBV(CBVRootSlot, rl::CreateDynamicConstantBuffer(&BloomApplyUniforms));
		Ctx.Dispatch(DivideRoundUp(ScreenDim.x, 8u), DivideRoundUp(ScreenDim.y, 8u), 1u);
	});
}

void BloomRenderPass_s::Resize(uint32_t Width, uint32_t Height)
{
	ScreenDim = uint2(Width, Height);

	while (true)
	{
		uint2 LastDim = DownsampledDims.empty() ? ScreenDim : DownsampledDims.back();
		uint2 NextDim = LastDim / 2u;
		if (NextDim.x < 4 || NextDim.y < 4)
		{
			break;
		}
		DownsampledDims.push_back(NextDim);
	}
}

void BloomRenderPass_s::DrawImGuiMenu()
{

}
