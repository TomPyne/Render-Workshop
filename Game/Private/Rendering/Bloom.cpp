#include "Bloom.h"

#include "Rendering/SpaceRenderer.h"

#include <Render/Render.h>
#include <RenderUtils/GPUContext/GPUContext.h>
#include <Shared/FileUtils/PathUtils.h>

void BloomRenderer_s::Init()
{
	rl::ShaderMacros Macros = { { "CBV_SLOT", SpaceRendererCBVRegister::CBV_DRAWCONSTANTS } };

	rl::ComputePipelineStateDesc PSODesc = {};
	PSODesc.RootSignatureOverride = SpaceRenderer_c::GetRootSignature();

	static const Path_s DownSamplePath = Path_s(PathDirectory_e::Shaders, L"Game", L"Bloom/BloomDownsample.hlsl");
	static const Path_s UpsamplePath = Path_s(PathDirectory_e::Shaders, L"Game", L"Bloom/BloomUpsample.hlsl");
	static const Path_s ApplyPath = Path_s(PathDirectory_e::Shaders, L"Game", L"Bloom/BloomApply.hlsl");

	PSODesc.Cs = rl::CreateComputeShader(DownSamplePath.ToString().c_str(), Macros);
	PSODesc.DebugName = L"BloomDownsampleCS";
	BloomDownsamplePSO = rl::CreateComputePipelineState(PSODesc);

	PSODesc.Cs = rl::CreateComputeShader(UpsamplePath.ToString().c_str(), Macros);
	PSODesc.DebugName = L"BloomUpsampleCS";
	BloomUpsamplePSO = rl::CreateComputePipelineState(PSODesc);

	PSODesc.Cs = rl::CreateComputeShader(ApplyPath.ToString().c_str(), Macros);
	PSODesc.DebugName = L"BloomApplyCS";
	BloomApplyPSO = rl::CreateComputePipelineState(PSODesc);

	Ready = BloomDownsamplePSO.IsValid() && BloomUpsamplePSO.IsValid() && BloomApplyPSO.IsValid();
}

RenderGraphResourceHandle_t BloomRenderer_s::DownsamplePass(RenderGraphBuilder_s& RGBuilder, RenderGraphResourceHandle_t Input, uint2 Dim, float Threshold)
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
			float Knee;
			float __Pad0[2];
		} DownsampleUniforms;

		DownsampleUniforms.InputTextureIndex = RG.GetSRVIndex(Input);
		DownsampleUniforms.OutputTextureIndex = RG.GetUAVIndex(Output);
		DownsampleUniforms.Dim = Dim;
		DownsampleUniforms.DimRcp = float2(1.0f / static_cast<float>(Dim.x), 1.0f / static_cast<float>(Dim.y));
		DownsampleUniforms.SrcDimRcp = float2(1.0f / static_cast<float>(SrcDim.x), 1.0f / static_cast<float>(SrcDim.y));
		DownsampleUniforms.Threshold = Threshold;
		DownsampleUniforms.Knee = Threshold > 0.0f ? 0.5f * Threshold : 0.0f;

		Ctx.SetRootSignature(SpaceRenderer_c::GetRootSignature());

		Ctx.SetComputeRootDescriptorTable(SpaceRendererRootSigSlots::RS_UAV_TABLE);
		Ctx.SetComputeRootDescriptorTable(SpaceRendererRootSigSlots::RS_SRV_TABLE);

		Ctx.SetPipelineState(BloomDownsamplePSO);

		Ctx.SetComputeRootCBV(SpaceRendererRootSigSlots::RS_DRAWCONSTANTS, RG.Alloc(DownsampleUniforms));

		Ctx.Dispatch(DivideRoundUp(Dim.x, 8u), DivideRoundUp(Dim.y, 8u), 1u);
	});

	return Output;
}

void BloomRenderer_s::UpsamplePass(RenderGraphBuilder_s& RGBuilder, RenderGraphResourceHandle_t Input, RenderGraphResourceHandle_t Output, uint2 Dim)
{
	const std::wstring ResourceName = L"BloomUpsampled_" + std::to_wstring(Dim.x) + L"x" + std::to_wstring(Dim.y);

	RGBuilder.AddPass(RenderGraphPassType_e::COMPUTE, ResourceName.c_str())
	.AccessResource(Input, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(Output, RenderGraphResourceAccessType_e::UAV, RenderGraphLoadOp_e::LOAD)
	.SetExecuteCallback([=](RenderGraph_s& RG, GPUContext_s& Ctx)
	{
		const uint2 InputDim = RG.GetTextureDimensions(Input);
		struct
		{
			uint32_t InputTextureIndex;
			uint32_t OutputTextureIndex;
			uint2 Dim;

			float2 DimRcp;
			float2 InputDimRcp;

			float FilterRadius;
			float __Pad0[3];
		} UpsampleUniforms;

		UpsampleUniforms.InputTextureIndex = RG.GetSRVIndex(Input);
		UpsampleUniforms.OutputTextureIndex = RG.GetUAVIndex(Output);
		UpsampleUniforms.Dim = Dim;
		UpsampleUniforms.DimRcp = float2(1.0f / static_cast<float>(Dim.x), 1.0f / static_cast<float>(Dim.y));
		UpsampleUniforms.InputDimRcp = float2(1.0f / static_cast<float>(InputDim.x), 1.0f / static_cast<float>(InputDim.y));
		UpsampleUniforms.FilterRadius = 1.0f;

		Ctx.SetRootSignature(SpaceRenderer_c::GetRootSignature());
		Ctx.SetComputeRootDescriptorTable(SpaceRendererRootSigSlots::RS_UAV_TABLE);
		Ctx.SetComputeRootDescriptorTable(SpaceRendererRootSigSlots::RS_SRV_TABLE);
		Ctx.SetPipelineState(BloomUpsamplePSO);
		Ctx.SetComputeRootCBV(SpaceRendererRootSigSlots::RS_DRAWCONSTANTS, RG.Alloc(UpsampleUniforms));
		Ctx.Dispatch(DivideRoundUp(Dim.x, 8u), DivideRoundUp(Dim.y, 8u), 1u);
	});
}

void BloomRenderer_s::AddPass(RenderGraphBuilder_s& RGBuilder, RenderGraphResourceHandle_t SceneColor, uint2 ScreenSize)
{
	std::vector<uint2> DownsampledDims;
	DownsampledDims.reserve(16);
	while (true)
	{
		uint2 LastDim = DownsampledDims.empty() ? ScreenSize : DownsampledDims.back();
		uint2 NextDim = LastDim / 2u;
		if (NextDim.x < 4 || NextDim.y < 4)
		{
			break;
		}
		DownsampledDims.push_back(NextDim);
	}

	if(DownsampledDims.empty())
		return;

	std::vector<RenderGraphResourceHandle_t> DownsampledTextures;
	DownsampledTextures.reserve(DownsampledDims.size());
	for (const uint2 DownSampleDim : DownsampledDims)
	{
		const bool FirstPass = DownsampledTextures.empty();
		DownsampledTextures.push_back(DownsamplePass(RGBuilder, FirstPass ? SceneColor : DownsampledTextures.back(), DownSampleDim, FirstPass ? 1.0f : -1.0f));
	}

	for (int32_t i = static_cast<int32_t>(DownsampledDims.size()) - 1; i >= 1; --i)
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
		BloomApplyUniforms.Dim = ScreenSize;
		BloomApplyUniforms.SrcDimRcp = float2(1.0f / static_cast<float>(ScreenSize.x), 1.0f / static_cast<float>(ScreenSize.y));
		BloomApplyUniforms.BloomIntensity = 0.1f;

		Ctx.SetRootSignature(SpaceRenderer_c::GetRootSignature());
		Ctx.SetComputeRootDescriptorTable(SpaceRendererRootSigSlots::RS_UAV_TABLE);
		Ctx.SetComputeRootDescriptorTable(SpaceRendererRootSigSlots::RS_SRV_TABLE);
		Ctx.SetPipelineState(BloomApplyPSO);
		Ctx.SetComputeRootCBV(SpaceRendererRootSigSlots::RS_DRAWCONSTANTS, RG.Alloc(BloomApplyUniforms));
		Ctx.Dispatch(DivideRoundUp(ScreenSize.x, 8u), DivideRoundUp(ScreenSize.y, 8u), 1u);
	});
}