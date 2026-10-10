#include "Rendering/DistanceFieldScene.h"

#include <Render/Render.h>

#include <algorithm>
#include <bit>

namespace
{

constexpr uint32_t kMinInstanceCapacity = 64;

}

DistanceFieldInstance_s MakeDistanceFieldInstance(const matrix& WorldMatrix, const AABB& VolumeBounds, uint32_t SDFTextureIndex, float MaxDistance)
{
	DistanceFieldInstance_s Instance = {};
	MakeDistanceFieldInstance(WorldMatrix, VolumeBounds, SDFTextureIndex, MaxDistance, Instance);
	return Instance;
}

void MakeDistanceFieldInstance(const matrix& WorldMatrix, const AABB& VolumeBounds, uint32_t SDFTextureIndex, float MaxDistance, DistanceFieldInstance_s& OutInstance)
{
	const float3 VolumeSize = VolumeBounds.maxs - VolumeBounds.mins;

	// Row vectors, so this applies the inverse world transform first, then maps the volume onto [0, 1]
	const matrix WorldToVolume = InverseMatrix(WorldMatrix)
		* MakeMatrixTranslation(-VolumeBounds.mins)
		* MakeMatrixScaling(1.0f / VolumeSize.x, 1.0f / VolumeSize.y, 1.0f / VolumeSize.z);

	const matrix ColumnWorldToVolume = TransposeMatrix(WorldToVolume);

	const AABB WorldBounds = VolumeBounds.GetTransformed(WorldMatrix);

	OutInstance.WorldToVolume[0] = ColumnWorldToVolume.r[0];
	OutInstance.WorldToVolume[1] = ColumnWorldToVolume.r[1];
	OutInstance.WorldToVolume[2] = ColumnWorldToVolume.r[2];
	OutInstance.WorldBoundsMin = WorldBounds.mins;
	OutInstance.SDFTextureIndex = SDFTextureIndex;
	OutInstance.WorldBoundsMax = WorldBounds.maxs;
	OutInstance.MaxDistance = MaxDistance;
	OutInstance.VolumeSize = VolumeSize;
	OutInstance.DistanceScale = Min(Min(Length(WorldMatrix.r[0].xyz), Length(WorldMatrix.r[1].xyz)), Length(WorldMatrix.r[2].xyz));
}

void DistanceFieldScene_c::Update(const std::vector<DistanceFieldInstance_s>& Instances)
{
	InstanceCount = static_cast<uint32_t>(Instances.size());
	if (InstanceCount == 0)
	{
		return;
	}

	if (InstanceCount > Capacity)
	{
		Capacity = Max(kMinInstanceCapacity, std::bit_ceil(InstanceCount));
		Staging.resize(Capacity);

		for (uint32_t BufferIt = 0; BufferIt < kBufferCount; BufferIt++)
		{
			Buffers[BufferIt] = rl::CreateStructuredBuffer(Staging.data(), Capacity);
			SRVs[BufferIt] = rl::CreateStructuredBufferSRV(Buffers[BufferIt], 0u, Capacity, static_cast<uint32_t>(sizeof(DistanceFieldInstance_s)));
		}
	}

	std::copy(Instances.begin(), Instances.end(), Staging.begin());

	WriteIndex = (WriteIndex + 1) % kBufferCount;
	rl::UpdateStructuredBufferFromArray(Buffers[WriteIndex], Staging.data(), Capacity);
}

uint32_t DistanceFieldScene_c::GetInstanceBufferSRVIndex() const
{
	return rl::GetDescriptorIndex(SRVs[WriteIndex]);
}
