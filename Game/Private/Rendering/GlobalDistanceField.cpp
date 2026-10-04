#include "Rendering/GlobalDistanceField.h"

#include "Rendering/DistanceFieldScene.h"

#include <Render/Render.h>
#include <RenderUtils/GPUContext/GPUContext.h>
#include <Shared/FileUtils/PathUtils.h>
#include <Shared/Logging/Logging.h>

#include <cmath>

void GlobalDistanceField_c::Init(rl::RootSignature_t InRootSignature, uint32_t InCBVRootSigSlot, uint32_t InUAVTableRootSigSlot, uint32_t InSRVTableRootSigSlot)
{
	RootSignature = rl::RootSignaturePtr::Ref(InRootSignature);
	CBVRootSigSlot = InCBVRootSigSlot;
	UAVTableRootSigSlot = InUAVTableRootSigSlot;
	SRVTableRootSigSlot = InSRVTableRootSigSlot;

	static const Path_s CompositeCSPath = Path_s(PathDirectory_e::Shaders, L"Game", L"DistanceFields/GlobalDistanceFieldComposite.hlsl");

	rl::ComputePipelineStateDesc PsoDesc = {};
	PsoDesc.Cs = rl::CreateComputeShader(CompositeCSPath.ToString().c_str());
	PsoDesc.RootSignatureOverride = RootSignature;
	PsoDesc.DebugName = L"GlobalDistanceFieldComposite";

	CompositePSO = rl::CreateComputePipelineState(PsoDesc);

	ASSERTMSG(CompositePSO.IsValid(), "Failed to create Global Distance Field Composite PSO");
}

RenderGraphResourceHandle_t GlobalDistanceField_c::AddPasses(RenderGraphBuilder_s& RGBuilder, const float3& CameraPos, const DistanceFieldScene_c& Scene)
{
	const uint32_t Resolution = Settings.Resolution;

	if (Volume && Volume->Desc.Width == Resolution && Settings.Freeze)
	{
		return RGBuilder.InjectTexture(Volume, L"GlobalDistanceField");
	}

	if (!Volume || Volume->Desc.Width != Resolution)
	{
		Volume = CreateRenderGraphTexture3D(Resolution, Resolution, Resolution, rl::RenderFormat::R16_FLOAT, RenderGraphResourceAccessType_e::UAV | RenderGraphResourceAccessType_e::SRV, L"GlobalDistanceField");
	}

	// Snapping to whole voxels keeps the content from swimming as the camera moves
	const float VoxelSize = GetVoxelSize();
	const float3 SnappedCentre = float3(roundf(CameraPos.x / VoxelSize), roundf(CameraPos.y / VoxelSize), roundf(CameraPos.z / VoxelSize)) * VoxelSize;
	const float3 HalfExtent = float3(Settings.Extent * 0.5f);
	VolumeBounds = AABB(SnappedCentre - HalfExtent, SnappedCentre + HalfExtent);

	RenderGraphResourceHandle_t VolumeTexture = RGBuilder.InjectTexture(Volume, L"GlobalDistanceField");

	const AABB Bounds = VolumeBounds;
	const float Band = GetBand();
	const uint32_t InstanceCount = Scene.GetInstanceCount();
	const uint32_t InstanceBufferIndex = InstanceCount > 0 ? Scene.GetInstanceBufferSRVIndex() : 0;

	RGBuilder.AddPass(RenderGraphPassType_e::COMPUTE, L"Global Distance Field Composite")
	.AccessResource(VolumeTexture, RenderGraphResourceAccessType_e::UAV, RenderGraphLoadOp_e::DONT_CARE)
	.SetExecuteCallback([=, this](RenderGraph_s& RG, GPUContext_s& Ctx)
	{
		struct CompositeUniforms_s
		{
			float3 VolumeMin;
			float VoxelSize;

			uint3 Resolution;
			float Band;

			uint32_t InstanceBufferIndex;
			uint32_t InstanceCount;
			uint32_t OutVolumeTexture;
			float __Pad;
		};
		static_assert(sizeof(CompositeUniforms_s) == 48, "Must match Uniforms_s in GlobalDistanceFieldComposite.hlsl");

		CompositeUniforms_s Uniforms = {};
		Uniforms.VolumeMin = Bounds.mins;
		Uniforms.VoxelSize = VoxelSize;
		Uniforms.Resolution = uint3(Resolution, Resolution, Resolution);
		Uniforms.Band = Band;
		Uniforms.InstanceBufferIndex = InstanceBufferIndex;
		Uniforms.InstanceCount = InstanceCount;
		Uniforms.OutVolumeTexture = RG.GetUAVIndex(VolumeTexture);

		Ctx.SetRootSignature(RootSignature);
		Ctx.SetComputeRootDescriptorTable(UAVTableRootSigSlot);
		Ctx.SetComputeRootDescriptorTable(SRVTableRootSigSlot);

		Ctx.SetPipelineState(CompositePSO);

		Ctx.SetComputeRootCBV(CBVRootSigSlot, RG.Alloc(Uniforms));

		Ctx.Dispatch(DivideRoundUp(Resolution, 4u), DivideRoundUp(Resolution, 4u), DivideRoundUp(Resolution, 4u));
	});

	return VolumeTexture;
}
