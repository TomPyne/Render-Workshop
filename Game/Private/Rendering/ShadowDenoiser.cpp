#include "Rendering/ShadowDenoiser.h"

#include "Rendering/SpaceRenderer.h"

#include <Render/Render.h>
#include <Render/TextureInfo.h>
#include <RenderUtils/GPUContext/GPUContext.h>
#include <Shared/FileUtils/PathUtils.h>
#include <Shared/Logging/Logging.h>

namespace
{

rl::RenderFormat GetHistoryRenderFormat(ShadowHistoryFormat_e Format)
{
	switch (Format)
	{
	case ShadowHistoryFormat_e::RGBA16Float:	return rl::RenderFormat::R16G16B16A16_FLOAT;
	case ShadowHistoryFormat_e::RGB10A2Unorm:	return rl::RenderFormat::R10G10B10A2_UNORM;
	}
	return rl::RenderFormat::R16G16B16A16_FLOAT;
}

constexpr rl::RenderFormat kLinearDepthFormat = rl::RenderFormat::R32_FLOAT;

}

void ShadowDenoiser_c::Init()
{
	static const Path_s TemporalCSPath = Path_s(PathDirectory_e::Shaders, L"Game", L"Shadows/ShadowTemporal.hlsl");

	rl::ComputePipelineStateDesc PsoDesc = {};
	PsoDesc.Cs = rl::CreateComputeShader(TemporalCSPath.ToString().c_str());
	PsoDesc.RootSignatureOverride = SpaceRenderer_c::GetRootSignature();
	PsoDesc.DebugName = L"ShadowTemporal";

	TemporalPSO = rl::CreateComputePipelineState(PsoDesc);

	ASSERTMSG(TemporalPSO.IsValid(), "Failed to create Shadow Temporal PSO");
}

void ShadowDenoiser_c::ResetHistory()
{
	HistoryValid = false;
}

ShadowDenoiseOutputs_s ShadowDenoiser_c::AddPasses(RenderGraphBuilder_s& RGBuilder, const ShadowDenoiseSettings_s& Settings, RenderGraphResourceHandle_t RawShadow,
	RenderGraphResourceHandle_t SceneDepth, RenderGraphResourceHandle_t SceneVelocity, RenderGraphResourceHandle_t SceneNormalRoughness,
	const matrix& InvViewProjection, uint2 Size)
{
	const bool SizeChanged = HistorySize.x != Size.x || HistorySize.y != Size.y;
	if (SizeChanged || HistoryFormat != Settings.HistoryFormat)
	{
		for (uint32_t HistoryIt = 0; HistoryIt < 2; HistoryIt++)
		{
			HistoryTextures[HistoryIt] = CreateRenderGraphTexture(Size.x, Size.y, GetHistoryRenderFormat(Settings.HistoryFormat), RenderGraphResourceAccessType_e::SRV_UAV, L"ShadowHistory");
			if (SizeChanged)
			{
				LinearDepthHistoryTextures[HistoryIt] = CreateRenderGraphTexture(Size.x, Size.y, kLinearDepthFormat, RenderGraphResourceAccessType_e::SRV_UAV, L"LinearDepthHistory");
			}
		}

		HistorySize = Size;
		HistoryFormat = Settings.HistoryFormat;
		HistoryValid = false;
	}

	const uint32_t HistoryWriteIndex = HistoryReadIndex ^ 1u;

	RenderGraphResourceHandle_t ShadowHistoryTexture = RGBuilder.InjectTexture(HistoryTextures[HistoryReadIndex], L"ShadowHistory");
	RenderGraphResourceHandle_t LinearDepthHistoryTexture = RGBuilder.InjectTexture(LinearDepthHistoryTextures[HistoryReadIndex], L"LinearDepthHistory");
	RenderGraphResourceHandle_t AccumulatedShadowTexture = RGBuilder.InjectTexture(HistoryTextures[HistoryWriteIndex], L"AccumulatedShadow");
	RenderGraphResourceHandle_t LinearDepthTexture = RGBuilder.InjectTexture(LinearDepthHistoryTextures[HistoryWriteIndex], L"LinearDepth");
	RenderGraphResourceHandle_t ShadowVarianceTexture = RGBuilder.CreateTexture(Size.x, Size.y, rl::RenderFormat::R16G16_FLOAT, RenderGraphResourceAccessType_e::SRV_UAV, L"ShadowVariance");

	const bool UseHistory = HistoryValid && Settings.TemporalEnabled;

	RGBuilder.AddPass(RenderGraphPassType_e::COMPUTE, L"Shadow Temporal Pass")
	.AccessResource(SceneDepth, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(SceneVelocity, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(SceneNormalRoughness, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(RawShadow, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(ShadowHistoryTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(LinearDepthHistoryTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(AccumulatedShadowTexture, RenderGraphResourceAccessType_e::UAV, RenderGraphLoadOp_e::DONT_CARE)
	.AccessResource(LinearDepthTexture, RenderGraphResourceAccessType_e::UAV, RenderGraphLoadOp_e::DONT_CARE)
	.AccessResource(ShadowVarianceTexture, RenderGraphResourceAccessType_e::UAV, RenderGraphLoadOp_e::DONT_CARE)
	.SetExecuteCallback([=, this](RenderGraph_s& RG, GPUContext_s& Ctx)
	{
		struct ShadowTemporalUniforms_s
		{
			matrix CamToWorld;

			uint2 ViewportSize;
			float2 InvViewportSize;

			uint32_t SceneDepthTexture;
			uint32_t VelocityTexture;
			uint32_t CurrentShadowTexture;
			uint32_t HistoryShadowTexture;

			uint32_t HistoryLinearDepthTexture;
			uint32_t OutShadowTexture;
			uint32_t OutLinearDepthTexture;
			uint32_t HistoryValid;

			float MaxConfidence;
			float ConfidenceRate;
			float DepthTolerance;
			uint32_t OutShadowVarianceTexture;

			uint32_t SceneNormalTexture;
			float MinConfidenceForTemporalVariance;
			float2 __Pad;
		};
		static_assert(sizeof(ShadowTemporalUniforms_s) == 144, "Must match Uniforms_s in ShadowTemporal.hlsl");

		ShadowTemporalUniforms_s Uniforms;
		Uniforms.CamToWorld = InvViewProjection;
		Uniforms.ViewportSize = Size;
		Uniforms.InvViewportSize = float2(1.0f / Size.x, 1.0f / Size.y);
		Uniforms.SceneDepthTexture = RG.GetSRVIndex(SceneDepth);
		Uniforms.VelocityTexture = RG.GetSRVIndex(SceneVelocity);
		Uniforms.CurrentShadowTexture = RG.GetSRVIndex(RawShadow);
		Uniforms.HistoryShadowTexture = RG.GetSRVIndex(ShadowHistoryTexture);
		Uniforms.HistoryLinearDepthTexture = RG.GetSRVIndex(LinearDepthHistoryTexture);
		Uniforms.OutShadowTexture = RG.GetUAVIndex(AccumulatedShadowTexture);
		Uniforms.OutLinearDepthTexture = RG.GetUAVIndex(LinearDepthTexture);
		Uniforms.HistoryValid = UseHistory ? 1u : 0u;
		Uniforms.MaxConfidence = Settings.MaxConfidence;
		Uniforms.ConfidenceRate = Settings.ConfidenceRate;
		Uniforms.DepthTolerance = Settings.DepthTolerance;
		Uniforms.OutShadowVarianceTexture = RG.GetUAVIndex(ShadowVarianceTexture);
		Uniforms.SceneNormalTexture = RG.GetSRVIndex(SceneNormalRoughness);
		Uniforms.MinConfidenceForTemporalVariance = Settings.MinConfidenceForTemporalVariance;
		Uniforms.__Pad = float2(0.0f, 0.0f);

		Ctx.SetRootSignature(SpaceRenderer_c::GetRootSignature());
		Ctx.SetComputeRootDescriptorTable(SpaceRendererRootSigSlots::RS_UAV_TABLE);
		Ctx.SetComputeRootDescriptorTable(SpaceRendererRootSigSlots::RS_SRV_TABLE);

		Ctx.SetPipelineState(TemporalPSO);

		Ctx.SetComputeRootCBV(SpaceRendererRootSigSlots::RS_DRAWCONSTANTS, RG.Alloc(Uniforms));

		Ctx.Dispatch(DivideRoundUp(Size.x, 8u), DivideRoundUp(Size.y, 8u), 1u);
	});

	ShadowDenoiseOutputs_s Outputs;
	Outputs.Shadow = ShadowVarianceTexture;
	Outputs.History = AccumulatedShadowTexture;
	return Outputs;
}

void ShadowDenoiser_c::EndFrame()
{
	HistoryReadIndex ^= 1u;
	HistoryValid = true;
}

uint64_t ShadowDenoiser_c::GetHistoryMemoryBytes() const
{
	const uint64_t PixelCount = static_cast<uint64_t>(HistorySize.x) * HistorySize.y;
	return 2u * PixelCount * rl::BitsPerPixel(GetHistoryRenderFormat(HistoryFormat)) / 8u;
}

uint64_t ShadowDenoiser_c::GetLinearDepthHistoryMemoryBytes() const
{
	const uint64_t PixelCount = static_cast<uint64_t>(HistorySize.x) * HistorySize.y;
	return 2u * PixelCount * rl::BitsPerPixel(kLinearDepthFormat) / 8u;
}
