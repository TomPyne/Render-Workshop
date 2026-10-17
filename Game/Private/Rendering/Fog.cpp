#include "Fog.h"

#include "Rendering/HeightFogInfo.h"
#include "Rendering/SpaceRenderer.h"

#include <Render/Render.h>
#include <RenderUtils/GPUContext/GPUContext.h>
#include <Shared/FileUtils/PathUtils.h>

void FogRenderer_s::Init()
{
	rl::ComputePipelineStateDesc PSODesc = {};
	PSODesc.RootSignatureOverride = SpaceRenderer_c::GetRootSignature();

	static const Path_s HeightFogPath = Path_s(PathDirectory_e::Shaders, L"Game", L"Fog/HeightFog.hlsl");

	PSODesc.Cs = rl::CreateComputeShader(HeightFogPath.ToString().c_str());
	PSODesc.DebugName = L"HeightFogCS";
	HeightFogPSO = rl::CreateComputePipelineState(PSODesc);

	Ready = HeightFogPSO.IsValid();
}

void FogRenderer_s::AddPass(RenderGraphBuilder_s& RGBuilder, const HeightFogInfo_s& Settings, const HeightFogInputs_s& Inputs)
{
	if (!Ready || !Settings.Enabled || Settings.Density <= 0.0f)
		return;

	const bool SkyEnabled = Inputs.SkyPrefilter != RenderGraphResourceHandle_t::NONE;

	RenderGraphPass_s& Pass = RGBuilder.AddPass(RenderGraphPassType_e::COMPUTE, L"Height Fog")
	.AccessResource(Inputs.SceneDepth, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(Inputs.SceneColor, RenderGraphResourceAccessType_e::UAV, RenderGraphLoadOp_e::LOAD)
	.SetExecuteCallback([=, this](RenderGraph_s& RG, GPUContext_s& Ctx)
	{
		struct HeightFogUniforms_s
		{
			matrix InvViewProjection;

			float3 CameraPosition;
			uint32_t SceneColorTextureIndex;

			float3 SunDirection;
			uint32_t SceneDepthTextureIndex;

			float3 SunInscatter;
			uint32_t SkyPrefilterTextureIndex;

			float3 AmbientLuminance;
			uint32_t SkyEnabled;

			float3 Albedo;
			float SkyIntensity;

			float Density;
			float HeightFalloff;
			float BaseHeight;
			float StartDistance;

			float MaxOpacity;
			float SunAnisotropy;
			float SkyPrefilterMaxMip;
			float SkyDistance;

			uint2 ScreenSize;
			float2 InvScreenSize;
		};
		static_assert(sizeof(HeightFogUniforms_s) == 192, "Must match Uniforms_s in HeightFog.hlsl");

		HeightFogUniforms_s Uniforms;
		Uniforms.InvViewProjection = Inputs.InvViewProjection;
		Uniforms.CameraPosition = Inputs.CameraPosition;
		Uniforms.SceneColorTextureIndex = RG.GetUAVIndex(Inputs.SceneColor);
		Uniforms.SunDirection = Inputs.SunDirection;
		Uniforms.SceneDepthTextureIndex = RG.GetSRVIndex(Inputs.SceneDepth);
		Uniforms.SunInscatter = Inputs.SunIrradiance * Settings.SunScatterScale;
		Uniforms.SkyPrefilterTextureIndex = SkyEnabled ? RG.GetSRVIndex(Inputs.SkyPrefilter) : 0u;
		Uniforms.AmbientLuminance = Inputs.AmbientLuminance;
		Uniforms.SkyEnabled = SkyEnabled ? 1u : 0u;
		Uniforms.Albedo = Settings.Albedo;
		Uniforms.SkyIntensity = Inputs.SkyIntensity;
		Uniforms.Density = Settings.Density;
		Uniforms.HeightFalloff = Settings.HeightFalloff;
		Uniforms.BaseHeight = Settings.BaseHeight;
		Uniforms.StartDistance = Settings.StartDistance;
		Uniforms.MaxOpacity = Settings.MaxOpacity;
		Uniforms.SunAnisotropy = Settings.SunAnisotropy;
		Uniforms.SkyPrefilterMaxMip = Inputs.SkyPrefilterMaxMip;
		Uniforms.SkyDistance = Settings.SkyDistance;
		Uniforms.ScreenSize = Inputs.ScreenSize;
		Uniforms.InvScreenSize = float2(1.0f / static_cast<float>(Inputs.ScreenSize.x), 1.0f / static_cast<float>(Inputs.ScreenSize.y));

		Ctx.SetRootSignature(SpaceRenderer_c::GetRootSignature());
		Ctx.SetComputeRootDescriptorTable(SpaceRendererRootSigSlots::RS_UAV_TABLE);
		Ctx.SetComputeRootDescriptorTable(SpaceRendererRootSigSlots::RS_SRV_TABLE);
		Ctx.SetPipelineState(HeightFogPSO);
		Ctx.SetComputeRootCBV(SpaceRendererRootSigSlots::RS_DRAWCONSTANTS, RG.Alloc(Uniforms));
		Ctx.Dispatch(DivideRoundUp(Inputs.ScreenSize.x, 8u), DivideRoundUp(Inputs.ScreenSize.y, 8u), 1u);
	});

	if (SkyEnabled)
	{
		Pass.AccessResource(Inputs.SkyPrefilter, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD);
	}
}
