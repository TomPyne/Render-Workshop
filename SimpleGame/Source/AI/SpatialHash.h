#pragma once

#include <SurfMath.h>

#include <vector>

// Uniform XZ grid hashed into a fixed bucket table, rebuilt from scratch each time the points move.
// Indices are the order points were added in.
class SpatialHash_c
{
public:
	void Reset();
	void Add(float3 Position);
	// Call after the last Add and before any Query
	void Finalise();

	// Appends every point within Radius of Position, which may include a point at Position itself
	void Query(float3 Position, float Radius, std::vector<size_t>& OutIndices) const;

	// Queries are cheapest when Radius is at most this
	float CellSize = 1.5f;

private:
	static constexpr u32 TableSize = 4096;

	int2 GetCell(float3 Position) const;
	static u32 GetBucket(int2 Cell);

	std::vector<float3> Points;
	std::vector<int2> Cells;
	// Bucket B owns Entries[BucketStart[B], BucketStart[B + 1])
	std::vector<u32> BucketStart;
	std::vector<u32> BucketFill;
	std::vector<size_t> Entries;
};
