#pragma once

#include <SurfMath.h>

#include <cstdint>
#include <vector>

struct VoxelizerInput_s
{
	const float3* Positions = nullptr;
	uint32_t VertexCount = 0;

	const uint32_t* Indices = nullptr;
	uint32_t IndexCount = 0;

	// Voxel count along the longest bounds axis, padding included. The other axes keep the same
	// voxel size, so they get fewer voxels.
	uint32_t Resolution = 64;
	uint32_t PaddingVoxels = 2;

	// Use the job system to voxelise in parallel by Z slice.
	bool Parallel = false;
};

struct VoxelizerOutput_s
{
	uint3 Dims = {};
	// Voxel centres sit at VolumeBounds.mins + (Index + 0.5) * VoxelSize.
	AABB VolumeBounds = {};
	float VoxelSize = 0.0f;
	// Signed world-space distance to the nearest triangle, negative inside, X fastest then Y then Z.
	std::vector<float> Distances;
};

bool VoxelizeGeometry(const VoxelizerInput_s& Input, VoxelizerOutput_s& Output);

// Maps distances to R8 with 128 at the surface and 1 and 255 at -MaxDistance and +MaxDistance,
// clamped beyond. Decode with (Value - 128) / 127 * MaxDistance.
std::vector<uint8_t> QuantizeDistances(const std::vector<float>& Distances, float MaxDistance);
