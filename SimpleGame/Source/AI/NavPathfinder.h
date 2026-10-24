#pragma once

#include <SurfMath.h>

#include <span>
#include <vector>

class NavGrid_c;

// Axis-aligned box in XZ, reached by any walkable cell whose centre is inside it.
// A box smaller than a cell, such as a point, is reached by the cell containing its centre, snapped if blocked.
struct NavGoal_s
{
	float3 Centre;
	float2 HalfExtents = float2(0.0f);
};

// A* over a NavGrid_c. Search buffers are kept between requests and invalidated by a per-search stamp, not cleared.
class NavPathfinder_c
{
public:
	// Stops at the first goal reached, which is the nearest by path. The start snaps to the nearest walkable cell
	// within SnapRadius. OutPath is the smoothed waypoints, start to goal.
	bool FindPath(const NavGrid_c& Grid, float3 Start, std::span<const NavGoal_s> Goals, std::vector<float3>& OutPath, int& OutGoalIndex);
	bool FindPath(const NavGrid_c& Grid, float3 Start, float3 Goal, std::vector<float3>& OutPath);

	const std::vector<int2>& GetLastRawPath() const { return RawPath; }
	int GetLastExpansions() const { return LastExpansions; }

	float SnapRadius = 5.0f;
	int MaxExpansions = 40000;

private:
	struct GoalCells_s
	{
		int2 Min;
		int2 Max;
		int GoalIndex;
	};

	bool SnapToCell(const NavGrid_c& Grid, float3 Position, int2& OutCell) const;
	bool ToGoalCells(const NavGrid_c& Grid, const NavGoal_s& Goal, int GoalIndex, GoalCells_s& OutCells) const;
	float Heuristic(int2 Cell) const;
	void Smooth(const NavGrid_c& Grid, std::vector<float3>& OutPath) const;

	struct Node_s
	{
		float G = 0.0f;
		int Parent = -1;
		u32 Stamp = 0;
		bool Closed = false;
	};

	struct OpenEntry_s
	{
		float F;
		float G;
		int Index;
	};

	std::vector<Node_s> Nodes;
	std::vector<OpenEntry_s> Open;
	std::vector<GoalCells_s> GoalCells;
	std::vector<int2> RawPath;
	u32 Stamp = 0;
	int LastExpansions = 0;
};
