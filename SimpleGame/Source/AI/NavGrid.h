#pragma once

#include <SurfMath.h>

#include <vector>

class Space_c;

// Walkability grid over XZ, built once from every NavObstacleComponent footprint. Cells are addressed as (X, Z).
class NavGrid_c
{
public:
	void Build(const Space_c& Space);

	bool IsInBounds(int2 Cell) const;
	// Out of bounds is not walkable
	bool IsWalkable(int2 Cell) const;
	float GetHeight(int2 Cell) const;

	int2 WorldToCell(float3 Position) const;
	// Cell centre at ground height
	float3 CellToWorld(int2 Cell) const;

	bool FindNearestWalkable(float3 Position, float MaxRadius, float3& OutPosition) const;
	bool HasLineOfSight(float3 From, float3 To) const;

	int2 GetSize() const { return Size; }

	// Set before Build
	float CellSize = 0.5f;
	float Extent = 200.0f;
	// Obstacles grow by the enemy radius so that a walkable cell centre keeps the whole enemy clear
	float Inflation = 0.5f;

private:
	void RasteriseObstacle(const matrix& WorldMatrix);

	int Index(int2 Cell) const { return Cell.y * Size.x + Cell.x; }

	float2 Origin;
	int2 Size;
	std::vector<u8> Blocked;
	std::vector<float> Heights;
};
