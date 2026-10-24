#include "NavGrid.h"

#include "Components/NavObstacleComponent.h"
#include "World/Ground.h"

#include <Object/SpatialObject.h>
#include <Shared/Logging/Logging.h>
#include <Space/Space.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>

void NavGrid_c::Build(const Space_c& Space)
{
	const int CellsPerSide = static_cast<int>(std::ceil(Extent / CellSize));
	Size = int2(CellsPerSide, CellsPerSide);
	Origin = float2(-0.5f * CellsPerSide * CellSize);

	Blocked.assign(static_cast<size_t>(Size.x) * Size.y, 0);
	Heights.resize(Blocked.size());

	for (int Z = 0; Z < Size.y; ++Z)
	{
		for (int X = 0; X < Size.x; ++X)
		{
			const float3 Centre = CellToWorld(int2(X, Z));
			Heights[Index(int2(X, Z))] = GetGroundHeight(Centre.x, Centre.z);
		}
	}

	std::vector<NavObstacleComponent_c*> Obstacles;
	Space.GetComponentsOfType(Obstacles);

	for (const NavObstacleComponent_c* Obstacle : Obstacles)
	{
		if (const SpatialObject_c* Owner = dynamic_cast<const SpatialObject_c*>(Obstacle->GetOwner()))
		{
			RasteriseObstacle(Owner->GetWorldMatrix());
		}
	}

	const size_t BlockedCount = std::count(Blocked.begin(), Blocked.end(), u8(1));
	LOGINFO("[NavGrid] Built %d x %d cells from %zu obstacles, %zu blocked", Size.x, Size.y, Obstacles.size(), BlockedCount);
}

void NavGrid_c::RasteriseObstacle(const matrix& WorldMatrix)
{
	// The cube mesh spans -1 to 1, so the scaled basis rows are the half extent vectors
	const float2 Centre(WorldMatrix.r[3].x, WorldMatrix.r[3].z);
	const float2 AxisX(WorldMatrix.r[0].x, WorldMatrix.r[0].z);
	const float2 AxisZ(WorldMatrix.r[2].x, WorldMatrix.r[2].z);

	const float HalfX = Length(AxisX);
	const float HalfZ = Length(AxisZ);
	if (HalfX <= 0.0f || HalfZ <= 0.0f)
		return;

	const float2 DirX(AxisX.x / HalfX, AxisX.y / HalfX);
	const float2 DirZ(AxisZ.x / HalfZ, AxisZ.y / HalfZ);
	const float LimitX = HalfX + Inflation;
	const float LimitZ = HalfZ + Inflation;

	const float BoundX = std::abs(DirX.x) * LimitX + std::abs(DirZ.x) * LimitZ;
	const float BoundZ = std::abs(DirX.y) * LimitX + std::abs(DirZ.y) * LimitZ;

	const int2 MinCell = WorldToCell(float3(Centre.x - BoundX, 0.0f, Centre.y - BoundZ));
	const int2 MaxCell = WorldToCell(float3(Centre.x + BoundX, 0.0f, Centre.y + BoundZ));

	for (int Z = Max(MinCell.y, 0); Z <= Min(MaxCell.y, Size.y - 1); ++Z)
	{
		for (int X = Max(MinCell.x, 0); X <= Min(MaxCell.x, Size.x - 1); ++X)
		{
			const float3 CellCentre = CellToWorld(int2(X, Z));
			const float2 Offset(CellCentre.x - Centre.x, CellCentre.z - Centre.y);

			if (std::abs(Dot(Offset, DirX)) <= LimitX && std::abs(Dot(Offset, DirZ)) <= LimitZ)
			{
				Blocked[Index(int2(X, Z))] = 1;
			}
		}
	}
}

bool NavGrid_c::IsInBounds(int2 Cell) const
{
	return Cell.x >= 0 && Cell.y >= 0 && Cell.x < Size.x && Cell.y < Size.y;
}

bool NavGrid_c::IsWalkable(int2 Cell) const
{
	return IsInBounds(Cell) && Blocked[Index(Cell)] == 0;
}

float NavGrid_c::GetHeight(int2 Cell) const
{
	return IsInBounds(Cell) ? Heights[Index(Cell)] : 0.0f;
}

int2 NavGrid_c::WorldToCell(float3 Position) const
{
	return int2(
		static_cast<int>(std::floor((Position.x - Origin.x) / CellSize)),
		static_cast<int>(std::floor((Position.z - Origin.y) / CellSize)));
}

float3 NavGrid_c::CellToWorld(int2 Cell) const
{
	const float X = Origin.x + (Cell.x + 0.5f) * CellSize;
	const float Z = Origin.y + (Cell.y + 0.5f) * CellSize;
	return float3(X, IsInBounds(Cell) ? Heights[Index(Cell)] : GetGroundHeight(X, Z), Z);
}

bool NavGrid_c::FindNearestWalkable(float3 Position, float MaxRadius, float3& OutPosition) const
{
	const int2 Start = WorldToCell(Position);
	const int MaxRing = static_cast<int>(std::ceil(MaxRadius / CellSize));
	const float MaxRadiusSq = MaxRadius * MaxRadius;

	float BestDistSq = FLT_MAX;
	int2 Best;
	bool Found = false;

	auto Consider = [&](int2 Cell)
	{
		if (!IsWalkable(Cell))
			return;

		const float3 Centre = CellToWorld(Cell);
		const float DX = Centre.x - Position.x;
		const float DZ = Centre.z - Position.z;
		const float DistSq = DX * DX + DZ * DZ;
		if (DistSq <= MaxRadiusSq && DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Cell;
			Found = true;
		}
	};

	// A hit on ring R can still be beaten by a cell up to R * sqrt(2) rings out, so the search runs on that far
	int StopRing = MaxRing;
	for (int Ring = 0; Ring <= StopRing; ++Ring)
	{
		if (Ring == 0)
		{
			Consider(Start);
		}
		else
		{
			for (int D = -Ring; D <= Ring; ++D)
			{
				Consider(int2(Start.x + D, Start.y - Ring));
				Consider(int2(Start.x + D, Start.y + Ring));
			}
			for (int D = -Ring + 1; D <= Ring - 1; ++D)
			{
				Consider(int2(Start.x - Ring, Start.y + D));
				Consider(int2(Start.x + Ring, Start.y + D));
			}
		}

		if (Found && StopRing == MaxRing)
		{
			StopRing = Min(MaxRing, static_cast<int>(std::ceil(Ring * 1.41421356f)) + 1);
		}
	}

	if (Found)
	{
		OutPosition = CellToWorld(Best);
	}
	return Found;
}

bool NavGrid_c::HasLineOfSight(float3 From, float3 To) const
{
	const float AX = (From.x - Origin.x) / CellSize;
	const float AZ = (From.z - Origin.y) / CellSize;
	const float BX = (To.x - Origin.x) / CellSize;
	const float BZ = (To.z - Origin.y) / CellSize;

	int2 Cell = WorldToCell(From);
	const int2 End = WorldToCell(To);

	const float DX = BX - AX;
	const float DZ = BZ - AZ;
	const int StepX = DX > 0.0f ? 1 : (DX < 0.0f ? -1 : 0);
	const int StepZ = DZ > 0.0f ? 1 : (DZ < 0.0f ? -1 : 0);

	constexpr float Never = FLT_MAX;
	const float TDeltaX = StepX != 0 ? std::abs(1.0f / DX) : Never;
	const float TDeltaZ = StepZ != 0 ? std::abs(1.0f / DZ) : Never;
	float TMaxX = StepX > 0 ? (Cell.x + 1 - AX) * TDeltaX : (StepX < 0 ? (AX - Cell.x) * TDeltaX : Never);
	float TMaxZ = StepZ > 0 ? (Cell.y + 1 - AZ) * TDeltaZ : (StepZ < 0 ? (AZ - Cell.y) * TDeltaZ : Never);

	const int Steps = std::abs(End.x - Cell.x) + std::abs(End.y - Cell.y);
	for (int Step = 0; Step < Steps; ++Step)
	{
		if (!IsWalkable(Cell))
			return false;

		if (TMaxX < TMaxZ)
		{
			TMaxX += TDeltaX;
			Cell.x += StepX;
		}
		else
		{
			TMaxZ += TDeltaZ;
			Cell.y += StepZ;
		}
	}

	return IsWalkable(Cell);
}
