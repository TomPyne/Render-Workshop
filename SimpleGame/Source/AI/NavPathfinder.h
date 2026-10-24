#pragma once

#include <SurfMath.h>

#include <vector>

class NavGrid_c;

// A* over a NavGrid_c. Search buffers are kept between requests and invalidated by a per-search stamp, not cleared.
class NavPathfinder_c
{
public:
	// Start and goal snap to the nearest walkable cell within SnapRadius. OutPath is the smoothed waypoints, start to goal.
	bool FindPath(const NavGrid_c& Grid, float3 Start, float3 Goal, std::vector<float3>& OutPath);

	const std::vector<int2>& GetLastRawPath() const { return RawPath; }
	int GetLastExpansions() const { return LastExpansions; }

	float SnapRadius = 5.0f;
	int MaxExpansions = 40000;

private:
	bool SnapToCell(const NavGrid_c& Grid, float3 Position, int2& OutCell) const;
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
	std::vector<int2> RawPath;
	u32 Stamp = 0;
	int LastExpansions = 0;
};
