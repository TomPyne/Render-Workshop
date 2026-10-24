#include "SpatialHash.h"

#include <cmath>

void SpatialHash_c::Reset()
{
	Points.clear();
	Cells.clear();
}

void SpatialHash_c::Add(float3 Position)
{
	Points.push_back(Position);
	Cells.push_back(GetCell(Position));
}

void SpatialHash_c::Finalise()
{
	// Counting sort into buckets
	BucketStart.assign(TableSize + 1, 0);
	for (const int2& Cell : Cells)
	{
		++BucketStart[GetBucket(Cell) + 1];
	}

	for (u32 Bucket = 0; Bucket < TableSize; ++Bucket)
	{
		BucketStart[Bucket + 1] += BucketStart[Bucket];
	}

	BucketFill.assign(BucketStart.begin(), BucketStart.end() - 1);
	Entries.resize(Points.size());
	for (size_t Index = 0; Index < Cells.size(); ++Index)
	{
		Entries[BucketFill[GetBucket(Cells[Index])]++] = Index;
	}
}

void SpatialHash_c::Query(float3 Position, float Radius, std::vector<size_t>& OutIndices) const
{
	if (Entries.empty())
		return;

	const int2 MinCell = GetCell(Position - float3(Radius, 0.0f, Radius));
	const int2 MaxCell = GetCell(Position + float3(Radius, 0.0f, Radius));
	const float RadiusSq = Radius * Radius;

	for (int Z = MinCell.y; Z <= MaxCell.y; ++Z)
	{
		for (int X = MinCell.x; X <= MaxCell.x; ++X)
		{
			const u32 Bucket = GetBucket(int2(X, Z));
			for (u32 Entry = BucketStart[Bucket]; Entry < BucketStart[Bucket + 1]; ++Entry)
			{
				const size_t Index = Entries[Entry];

				// Other cells can share the bucket, and would be reported twice when they are also in range
				if (Cells[Index].x != X || Cells[Index].y != Z)
					continue;

				const float DX = Points[Index].x - Position.x;
				const float DZ = Points[Index].z - Position.z;
				if (DX * DX + DZ * DZ <= RadiusSq)
				{
					OutIndices.push_back(Index);
				}
			}
		}
	}
}

int2 SpatialHash_c::GetCell(float3 Position) const
{
	return int2(static_cast<int>(std::floor(Position.x / CellSize)), static_cast<int>(std::floor(Position.z / CellSize)));
}

u32 SpatialHash_c::GetBucket(int2 Cell)
{
	return ((static_cast<u32>(Cell.x) * 73856093u) ^ (static_cast<u32>(Cell.y) * 19349663u)) & (TableSize - 1);
}
