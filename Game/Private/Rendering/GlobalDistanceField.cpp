#include "Rendering/GlobalDistanceField.h"

#include "Rendering/DistanceFieldScene.h"

#include <Render/Render.h>
#include <RenderUtils/GPUContext/GPUContext.h>
#include <Shared/FileUtils/PathUtils.h>
#include <Shared/Logging/Logging.h>

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>

namespace
{

constexpr uint32_t kMinBufferCapacity = 64;

}

void FrameStructuredBuffer_c::Update(const void* Data, uint32_t Count)
{
	if (Count == 0)
	{
		return;
	}

	if (Count > Capacity)
	{
		Capacity = Max(kMinBufferCapacity, std::bit_ceil(Count));
		Staging.resize(static_cast<size_t>(Capacity) * Stride);

		for (uint32_t BufferIt = 0; BufferIt < kBufferCount; BufferIt++)
		{
			Buffers[BufferIt] = rl::CreateStructuredBuffer(Staging.data(), Staging.size(), Stride, rl::RenderResourceFlags::SRV);
			SRVs[BufferIt] = rl::CreateStructuredBufferSRV(Buffers[BufferIt], 0u, Capacity, Stride);
		}
	}

	memcpy(Staging.data(), Data, static_cast<size_t>(Count) * Stride);

	WriteIndex = (WriteIndex + 1) % kBufferCount;
	rl::UpdateStructuredBuffer(Buffers[WriteIndex], Staging.data(), Staging.size());
}

uint32_t FrameStructuredBuffer_c::GetSRVIndex() const
{
	return rl::GetDescriptorIndex(SRVs[WriteIndex]);
}

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

	const bool UseBrickCulling = Settings.BrickCulling && InstanceCount > 0;
	uint32_t BrickRangesBufferIndex = 0;
	uint32_t BrickInstancesBufferIndex = 0;
	if (UseBrickCulling)
	{
		BinInstances(Scene);

		BrickRangesBufferIndex = BrickRangesBuffer.GetSRVIndex();
		BrickInstancesBufferIndex = BrickInstances.empty() ? 0 : BrickInstancesBuffer.GetSRVIndex();
	}
	else
	{
		BrickStats = {};
	}

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
			uint32_t BricksPerAxis;

			uint32_t BrickRangesBufferIndex;
			uint32_t BrickInstancesBufferIndex;
			uint32_t UseBrickCulling;
			float __Pad;
		};
		static_assert(sizeof(CompositeUniforms_s) == 64, "Must match Uniforms_s in GlobalDistanceFieldComposite.hlsl");

		CompositeUniforms_s Uniforms = {};
		Uniforms.VolumeMin = Bounds.mins;
		Uniforms.VoxelSize = VoxelSize;
		Uniforms.Resolution = uint3(Resolution, Resolution, Resolution);
		Uniforms.Band = Band;
		Uniforms.InstanceBufferIndex = InstanceBufferIndex;
		Uniforms.InstanceCount = InstanceCount;
		Uniforms.OutVolumeTexture = RG.GetUAVIndex(VolumeTexture);
		Uniforms.BricksPerAxis = Resolution / kBrickSize;
		Uniforms.BrickRangesBufferIndex = BrickRangesBufferIndex;
		Uniforms.BrickInstancesBufferIndex = BrickInstancesBufferIndex;
		Uniforms.UseBrickCulling = UseBrickCulling ? 1u : 0u;

		Ctx.SetRootSignature(RootSignature);
		Ctx.SetComputeRootDescriptorTable(UAVTableRootSigSlot);
		Ctx.SetComputeRootDescriptorTable(SRVTableRootSigSlot);

		Ctx.SetPipelineState(CompositePSO);

		Ctx.SetComputeRootCBV(CBVRootSigSlot, RG.Alloc(Uniforms));

		Ctx.Dispatch(DivideRoundUp(Resolution, 4u), DivideRoundUp(Resolution, 4u), DivideRoundUp(Resolution, 4u));
	});

	return VolumeTexture;
}

void GlobalDistanceField_c::BinInstances(const DistanceFieldScene_c& Scene)
{
	const int32_t BricksPerAxis = static_cast<int32_t>(Settings.Resolution / kBrickSize);
	const uint32_t BrickCount = static_cast<uint32_t>(BricksPerAxis * BricksPerAxis * BricksPerAxis);
	const float BrickWorldSize = GetVoxelSize() * kBrickSize;
	const float Band = GetBand();
	const AABB& Bounds = VolumeBounds;

	const std::vector<DistanceFieldInstance_s>& Instances = Scene.GetInstances();
	const uint32_t InstanceCount = Scene.GetInstanceCount();

	// Inclusive brick range per instance, Min.x > Max.x when the instance misses the volume
	struct BrickRange_s
	{
		int32_t Min[3];
		int32_t Max[3];
	};
	std::vector<BrickRange_s> InstanceBricks(InstanceCount);

	BrickRanges.assign(BrickCount, uint2(0u, 0u));

	auto BrickIndex = [BricksPerAxis](int32_t X, int32_t Y, int32_t Z)
	{
		return static_cast<uint32_t>(X + (Y + Z * BricksPerAxis) * BricksPerAxis);
	};

	for (uint32_t InstanceIt = 0; InstanceIt < InstanceCount; InstanceIt++)
	{
		const DistanceFieldInstance_s& Instance = Instances[InstanceIt];
		BrickRange_s& Range = InstanceBricks[InstanceIt];
		Range = { { 1, 0, 0 }, { 0, 0, 0 } };

		// Voxels at least a band away from the instance bounds skip it in the shader, so only the grown bounds matter
		const float GrownMin[3] = { Instance.WorldBoundsMin.x - Band, Instance.WorldBoundsMin.y - Band, Instance.WorldBoundsMin.z - Band };
		const float GrownMax[3] = { Instance.WorldBoundsMax.x + Band, Instance.WorldBoundsMax.y + Band, Instance.WorldBoundsMax.z + Band };
		const float VolumeMin[3] = { Bounds.mins.x, Bounds.mins.y, Bounds.mins.z };
		const float VolumeMax[3] = { Bounds.maxs.x, Bounds.maxs.y, Bounds.maxs.z };

		bool Overlaps = true;
		int32_t BrickMin[3];
		int32_t BrickMax[3];
		for (int32_t Axis = 0; Axis < 3; Axis++)
		{
			if (GrownMax[Axis] < VolumeMin[Axis] || GrownMin[Axis] > VolumeMax[Axis])
			{
				Overlaps = false;
				break;
			}

			BrickMin[Axis] = Clamp(static_cast<int32_t>(floorf((GrownMin[Axis] - VolumeMin[Axis]) / BrickWorldSize)), 0, BricksPerAxis - 1);
			BrickMax[Axis] = Clamp(static_cast<int32_t>(floorf((GrownMax[Axis] - VolumeMin[Axis]) / BrickWorldSize)), 0, BricksPerAxis - 1);
		}

		if (!Overlaps)
		{
			continue;
		}

		Range = { { BrickMin[0], BrickMin[1], BrickMin[2] }, { BrickMax[0], BrickMax[1], BrickMax[2] } };

		for (int32_t Z = BrickMin[2]; Z <= BrickMax[2]; Z++)
		for (int32_t Y = BrickMin[1]; Y <= BrickMax[1]; Y++)
		for (int32_t X = BrickMin[0]; X <= BrickMax[0]; X++)
		{
			BrickRanges[BrickIndex(X, Y, Z)].y++;
		}
	}

	GlobalDistanceFieldBrickStats_s Stats = {};
	Stats.BrickCount = BrickCount;
	Stats.MinInstances = UINT32_MAX;

	uint32_t Offset = 0;
	for (uint2& Range : BrickRanges)
	{
		Range.x = Offset;
		Offset += Range.y;

		if (Range.y == 0)
		{
			Stats.EmptyBrickCount++;
			continue;
		}

		Stats.MinInstances = Min(Stats.MinInstances, Range.y);
		Stats.MaxInstances = Max(Stats.MaxInstances, Range.y);

		// Reset so the fill below can use it as a cursor
		Range.y = 0;
	}

	const uint32_t NonEmptyBrickCount = BrickCount - Stats.EmptyBrickCount;
	Stats.TotalEntries = Offset;
	Stats.MinInstances = NonEmptyBrickCount > 0 ? Stats.MinInstances : 0;
	Stats.AverageInstances = NonEmptyBrickCount > 0 ? static_cast<float>(Offset) / NonEmptyBrickCount : 0.0f;
	BrickStats = Stats;

	BrickInstances.resize(Offset);

	for (uint32_t InstanceIt = 0; InstanceIt < InstanceCount; InstanceIt++)
	{
		const BrickRange_s& Range = InstanceBricks[InstanceIt];

		for (int32_t Z = Range.Min[2]; Z <= Range.Max[2]; Z++)
		for (int32_t Y = Range.Min[1]; Y <= Range.Max[1]; Y++)
		for (int32_t X = Range.Min[0]; X <= Range.Max[0]; X++)
		{
			uint2& BrickRange = BrickRanges[BrickIndex(X, Y, Z)];
			BrickInstances[BrickRange.x + BrickRange.y] = InstanceIt;
			BrickRange.y++;
		}
	}

	BrickRangesBuffer.Update(BrickRanges.data(), BrickCount);
	BrickInstancesBuffer.Update(BrickInstances.data(), static_cast<uint32_t>(BrickInstances.size()));
}
