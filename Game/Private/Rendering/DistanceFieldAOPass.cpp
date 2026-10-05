#include "Rendering/DistanceFieldAOPass.h"

#include "Rendering/GlobalDistanceField.h"
#include "Rendering/SpaceRenderer.h"

#include <Render/Render.h>
#include <RenderUtils/GPUContext/GPUContext.h>
#include <Shared/FileUtils/PathUtils.h>
#include <Shared/Logging/Logging.h>

#include <cmath>

void DistanceFieldAORenderer_s::Init()
{
	static const Path_s AOCSPath = Path_s(PathDirectory_e::Shaders, L"Game", L"DistanceFields/DistanceFieldAO.hlsl");

	rl::ComputePipelineStateDesc PsoDesc = {};
	PsoDesc.Cs = rl::CreateComputeShader(AOCSPath.ToString().c_str());
	PsoDesc.RootSignatureOverride = SpaceRenderer_c::GetRootSignature();
	PsoDesc.DebugName = L"DistanceFieldAO";

	PSO = rl::CreateComputePipelineState(PsoDesc);

	ASSERTMSG(PSO.IsValid(), "Failed to create Distance Field AO PSO");
}

RenderGraphResourceHandle_t DistanceFieldAORenderer_s::AddPass(RenderGraphBuilder_s& RGBuilder, const DistanceFieldAOSettings_s& Settings, const GlobalDistanceField_c& GlobalDistanceField,
	RenderGraphResourceHandle_t GlobalVolume, RenderGraphResourceHandle_t SceneDepth, RenderGraphResourceHandle_t SceneNormalRoughness, const matrix& InvViewProjection, uint2 OutputSize)
{
	RenderGraphResourceHandle_t Output = RGBuilder.CreateTexture(OutputSize.x, OutputSize.y, rl::RenderFormat::R8_UNORM, RenderGraphResourceAccessType_e::UAV | RenderGraphResourceAccessType_e::SRV, L"DistanceFieldAO");

	const bool Enabled = Settings.Enabled && GlobalVolume != RenderGraphResourceHandle_t::NONE;

	const AABB& VolumeBounds = GlobalDistanceField.GetVolumeBounds();
	const float VoxelSize = GlobalDistanceField.GetVoxelSize();
	const uint32_t StepsPerCone = Max(Settings.StepsPerCone, 2u);
	const float FirstStep = VoxelSize;
	const float MaxDistance = Max(Settings.MaxDistance, FirstStep);
	const float StepScale = std::pow(MaxDistance / FirstStep, 1.0f / static_cast<float>(StepsPerCone - 1));

	// The render graph skips NONE handles, so a missing volume needs no special casing here
	RGBuilder.AddPass(RenderGraphPassType_e::COMPUTE, L"Distance Field AO")
	.AccessResource(GlobalVolume, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(SceneDepth, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(SceneNormalRoughness, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(Output, RenderGraphResourceAccessType_e::UAV, RenderGraphLoadOp_e::DONT_CARE)
	.SetExecuteCallback([=, this](RenderGraph_s& RG, GPUContext_s& Ctx)
	{
		struct AOUniforms_s
		{
			matrix InvViewProjection;

			uint2 OutputSize;
			uint32_t SceneDepthTexture;
			uint32_t SceneNormalTexture;

			float3 VolumeMin;
			uint32_t VolumeTexture;

			float3 VolumeMax;
			float Band;

			uint32_t OutputTexture;
			uint32_t Enabled;
			uint32_t StepsPerCone;
			float FirstStep;

			float StepScale;
			float NormalBias;
			float Power;
			float __Pad;
		};
		static_assert(sizeof(AOUniforms_s) == 144, "Must match Uniforms_s in DistanceFieldAO.hlsl");

		AOUniforms_s Uniforms = {};
		Uniforms.InvViewProjection = InvViewProjection;
		Uniforms.OutputSize = OutputSize;
		Uniforms.SceneDepthTexture = RG.GetSRVIndex(SceneDepth);
		Uniforms.SceneNormalTexture = RG.GetSRVIndex(SceneNormalRoughness);
		Uniforms.VolumeMin = VolumeBounds.mins;
		Uniforms.VolumeTexture = Enabled ? RG.GetSRVIndex(GlobalVolume) : 0u;
		Uniforms.VolumeMax = VolumeBounds.maxs;
		Uniforms.Band = GlobalDistanceField.GetBand();
		Uniforms.OutputTexture = RG.GetUAVIndex(Output);
		Uniforms.Enabled = Enabled ? 1u : 0u;
		Uniforms.StepsPerCone = StepsPerCone;
		Uniforms.FirstStep = FirstStep;
		Uniforms.StepScale = StepScale;
		Uniforms.NormalBias = Settings.NormalBiasVoxels * VoxelSize;
		Uniforms.Power = Settings.Power;

		Ctx.SetRootSignature(SpaceRenderer_c::GetRootSignature());
		Ctx.SetComputeRootDescriptorTable(SpaceRendererRootSigSlots::RS_UAV_TABLE);
		Ctx.SetComputeRootDescriptorTable(SpaceRendererRootSigSlots::RS_SRV_TABLE);

		Ctx.SetPipelineState(PSO);

		Ctx.SetComputeRootCBV(SpaceRendererRootSigSlots::RS_DRAWCONSTANTS, RG.Alloc(Uniforms));

		Ctx.Dispatch(DivideRoundUp(OutputSize.x, 8u), DivideRoundUp(OutputSize.y, 8u), 1u);
	});

	return Output;
}
