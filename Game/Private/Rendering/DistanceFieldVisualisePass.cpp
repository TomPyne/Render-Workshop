#include "Rendering/DistanceFieldVisualisePass.h"

#include "Rendering/GlobalDistanceField.h"

#include <Render/Render.h>
#include <RenderUtils/GPUContext/GPUContext.h>
#include <Shared/FileUtils/PathUtils.h>
#include <Shared/Logging/Logging.h>

void DistanceFieldVisualiseRenderer_s::Init(rl::RootSignature_t InRootSignature, uint32_t InCBVRootSigSlot, uint32_t InUAVTableRootSigSlot, uint32_t InSRVTableRootSigSlot)
{
	RootSignature = rl::RootSignaturePtr::Ref(InRootSignature);
	CBVRootSigSlot = InCBVRootSigSlot;
	UAVTableRootSigSlot = InUAVTableRootSigSlot;
	SRVTableRootSigSlot = InSRVTableRootSigSlot;

	static const Path_s VisualiseCSPath = Path_s(PathDirectory_e::Shaders, L"Game", L"DistanceFields/DistanceFieldVisualise.hlsl");

	rl::ComputePipelineStateDesc PsoDesc = {};
	PsoDesc.Cs = rl::CreateComputeShader(VisualiseCSPath.ToString().c_str());
	PsoDesc.RootSignatureOverride = RootSignature;
	PsoDesc.DebugName = L"DistanceFieldVisualise";

	PSO = rl::CreateComputePipelineState(PsoDesc);

	ASSERTMSG(PSO.IsValid(), "Failed to create Distance Field Visualise PSO");
}

RenderGraphResourceHandle_t DistanceFieldVisualiseRenderer_s::AddPass(RenderGraphBuilder_s& RGBuilder, DistanceFieldVisualiseMode_e Mode, const DistanceFieldVisualiseSettings_s& Settings,
	const GlobalDistanceField_c& GlobalDistanceField, RenderGraphResourceHandle_t GlobalVolume, const matrix& InvViewProjection, uint2 OutputSize)
{
	RenderGraphResourceHandle_t Output = RGBuilder.CreateTexture(OutputSize.x, OutputSize.y, rl::RenderFormat::R8G8B8A8_UNORM, RenderGraphResourceAccessType_e::UAV | RenderGraphResourceAccessType_e::SRV, L"DistanceFieldVisualise");

	const uint32_t Resolution = GlobalDistanceField.Settings.Resolution;
	const float3 VolumeMin = GlobalDistanceField.GetVolumeBounds().mins;
	const float VoxelSize = GlobalDistanceField.GetVoxelSize();
	const float Band = GlobalDistanceField.GetBand();
	const uint32_t SliceIndex = Min(Settings.SliceIndex, Resolution - 1);

	float GridCellSize = 0.0f;
	switch (Settings.Grid)
	{
	case DistanceFieldGridMode_e::None:		GridCellSize = 0.0f; break;
	case DistanceFieldGridMode_e::Voxel:	GridCellSize = VoxelSize; break;
	case DistanceFieldGridMode_e::Brick:	GridCellSize = VoxelSize * GlobalDistanceField_c::kBrickSize; break;
	}

	RGBuilder.AddPass(RenderGraphPassType_e::COMPUTE, L"Distance Field Visualise")
	.AccessResource(GlobalVolume, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(Output, RenderGraphResourceAccessType_e::UAV, RenderGraphLoadOp_e::DONT_CARE)
	.SetExecuteCallback([=, this](RenderGraph_s& RG, GPUContext_s& Ctx)
	{
		struct VisualiseUniforms_s
		{
			matrix InvViewProjection;

			uint2 OutputSize;
			uint32_t OutputTexture;
			uint32_t Mode;

			float3 VolumeMin;
			float VoxelSize;

			uint3 VolumeResolution;
			float Band;

			uint32_t VolumeTexture;
			uint32_t SliceIndex;
			uint32_t MaxSteps;
			uint32_t TraceView;

			uint32_t ExitTint;
			float GridCellSize;
			float2 __Pad;
		};
		static_assert(sizeof(VisualiseUniforms_s) == 144, "Must match Uniforms_s in DistanceFieldVisualise.hlsl");

		VisualiseUniforms_s Uniforms = {};
		Uniforms.InvViewProjection = InvViewProjection;
		Uniforms.OutputSize = OutputSize;
		Uniforms.OutputTexture = RG.GetUAVIndex(Output);
		Uniforms.Mode = static_cast<uint32_t>(Mode);
		Uniforms.VolumeMin = VolumeMin;
		Uniforms.VoxelSize = VoxelSize;
		Uniforms.VolumeResolution = uint3(Resolution, Resolution, Resolution);
		Uniforms.Band = Band;
		Uniforms.VolumeTexture = RG.GetSRVIndex(GlobalVolume);
		Uniforms.SliceIndex = SliceIndex;
		Uniforms.MaxSteps = Max(Settings.MaxSteps, 1u);
		Uniforms.TraceView = static_cast<uint32_t>(Settings.TraceView);
		Uniforms.ExitTint = Settings.ExitTint ? 1u : 0u;
		Uniforms.GridCellSize = GridCellSize;

		Ctx.SetRootSignature(RootSignature);
		Ctx.SetComputeRootDescriptorTable(UAVTableRootSigSlot);
		Ctx.SetComputeRootDescriptorTable(SRVTableRootSigSlot);

		Ctx.SetPipelineState(PSO);

		Ctx.SetComputeRootCBV(CBVRootSigSlot, RG.Alloc(Uniforms));

		Ctx.Dispatch(DivideRoundUp(OutputSize.x, 8u), DivideRoundUp(OutputSize.y, 8u), 1u);
	});

	return Output;
}
