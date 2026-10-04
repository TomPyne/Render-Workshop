#pragma once

#include <Render/RenderTypes.h>
#include <Render/View.h>
#include <SurfMath.h>

#include <cstdint>
#include <vector>

// One mesh SDF placed in the world. Must match DistanceFieldInstance_s in the shaders.
struct DistanceFieldInstance_s
{
	// Column vector rows, so UVW = float3(dot(Row, float4(WorldPos, 1))), mapping the padded SDF
	// volume onto [0, 1]
	float4 WorldToVolume[3];

	float3 WorldBoundsMin;
	uint32_t SDFTextureIndex;

	float3 WorldBoundsMax;
	// Texel values decode to mesh space distance as (Value * 255 - 128) / 127 * MaxDistance
	float MaxDistance;

	// Mesh space size of the padded volume, for distances measured outside it
	float3 VolumeSize;
	// Smallest axis scale of the world matrix, converts mesh space distances to a conservative world distance
	float DistanceScale;
};
static_assert(sizeof(DistanceFieldInstance_s) == 96, "Must match DistanceFieldInstance_s in the shaders");

// Builds the instance for an SDF with mesh space VolumeBounds placed at WorldMatrix
DistanceFieldInstance_s MakeDistanceFieldInstance(const matrix& WorldMatrix, const AABB& VolumeBounds, uint32_t SDFTextureIndex, float MaxDistance);

// GPU copy of the frame's distance field instances
class DistanceFieldScene_c
{
public:
	void Update(const std::vector<DistanceFieldInstance_s>& Instances);

	// Descriptor index of this frame's instance buffer, only valid when GetInstanceCount() > 0
	uint32_t GetInstanceBufferSRVIndex() const;
	uint32_t GetInstanceCount() const { return InstanceCount; }
	uint32_t GetCapacity() const { return Capacity; }

	// CPU copy of this frame's instances, the first GetInstanceCount() entries are valid
	const std::vector<DistanceFieldInstance_s>& GetInstances() const { return Staging; }

private:
	// A buffer update is copied when the frame's command list runs, so a buffer can only be
	// rewritten once the frame that last used it has finished. One per frame in flight.
	static constexpr uint32_t kBufferCount = rl::RenderView::NumBackBuffers;

	rl::StructuredBufferPtr Buffers[kBufferCount] = {};
	rl::ShaderResourceViewPtr SRVs[kBufferCount] = {};

	// Sized to Capacity, since an update always copies the whole buffer
	std::vector<DistanceFieldInstance_s> Staging;

	uint32_t Capacity = 0;
	uint32_t InstanceCount = 0;
	uint32_t WriteIndex = 0;
};
