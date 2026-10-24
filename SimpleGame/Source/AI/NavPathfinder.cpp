#include "NavPathfinder.h"

#include "AI/NavGrid.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace
{
	constexpr float Sqrt2 = 1.41421356f;

	// Exact cost of an unobstructed 8-way path, in cells
	float OctileDistance(int DX, int DZ)
	{
		return static_cast<float>(DX + DZ) + (Sqrt2 - 2.0f) * static_cast<float>(Min(DX, DZ));
	}

	int DistanceToRange(int Value, int RangeMin, int RangeMax)
	{
		return Max(Max(RangeMin - Value, Value - RangeMax), 0);
	}
}

bool NavPathfinder_c::SnapToCell(const NavGrid_c& Grid, float3 Position, int2& OutCell) const
{
	OutCell = Grid.WorldToCell(Position);
	if (Grid.IsWalkable(OutCell))
		return true;

	float3 Snapped;
	if (!Grid.FindNearestWalkable(Position, SnapRadius, Snapped))
		return false;

	OutCell = Grid.WorldToCell(Snapped);
	return true;
}

bool NavPathfinder_c::ToGoalCells(const NavGrid_c& Grid, const NavGoal_s& Goal, int GoalIndex, GoalCells_s& OutCells) const
{
	// Cells whose centre lies inside the box, offset by half a cell so WorldToCell rounds to them
	const float HalfCell = 0.5f * Grid.CellSize;
	const float3 Low(Goal.Centre.x - Goal.HalfExtents.x + HalfCell, 0.0f, Goal.Centre.z - Goal.HalfExtents.y + HalfCell);
	const float3 High(Goal.Centre.x + Goal.HalfExtents.x - HalfCell, 0.0f, Goal.Centre.z + Goal.HalfExtents.y - HalfCell);

	OutCells.Min = Grid.WorldToCell(Low);
	OutCells.Max = Grid.WorldToCell(High);
	OutCells.GoalIndex = GoalIndex;

	if (OutCells.Min.x > OutCells.Max.x || OutCells.Min.y > OutCells.Max.y)
	{
		int2 Cell;
		if (!SnapToCell(Grid, Goal.Centre, Cell))
			return false;

		OutCells.Min = Cell;
		OutCells.Max = Cell;
		return true;
	}

	const int2 Size = Grid.GetSize();
	OutCells.Min = int2(Max(OutCells.Min.x, 0), Max(OutCells.Min.y, 0));
	OutCells.Max = int2(Min(OutCells.Max.x, Size.x - 1), Min(OutCells.Max.y, Size.y - 1));
	return OutCells.Min.x <= OutCells.Max.x && OutCells.Min.y <= OutCells.Max.y;
}

float NavPathfinder_c::Heuristic(int2 Cell) const
{
	// The minimum over goals stays admissible, so the first goal reached is the nearest by path
	float Best = FLT_MAX;
	for (const GoalCells_s& Goal : GoalCells)
	{
		const int DX = DistanceToRange(Cell.x, Goal.Min.x, Goal.Max.x);
		const int DZ = DistanceToRange(Cell.y, Goal.Min.y, Goal.Max.y);
		Best = Min(Best, OctileDistance(DX, DZ));
	}
	return Best;
}

bool NavPathfinder_c::FindPath(const NavGrid_c& Grid, float3 Start, float3 Goal, std::vector<float3>& OutPath)
{
	const NavGoal_s PointGoal{ Goal };
	int GoalIndex = -1;
	return FindPath(Grid, Start, std::span<const NavGoal_s>(&PointGoal, 1), OutPath, GoalIndex);
}

bool NavPathfinder_c::FindPath(const NavGrid_c& Grid, float3 Start, std::span<const NavGoal_s> Goals, std::vector<float3>& OutPath, int& OutGoalIndex)
{
	OutPath.clear();
	RawPath.clear();
	LastExpansions = 0;
	OutGoalIndex = -1;

	int2 StartCell;
	if (!SnapToCell(Grid, Start, StartCell))
		return false;

	GoalCells.clear();
	for (size_t Index = 0; Index < Goals.size(); ++Index)
	{
		GoalCells_s Cells;
		if (ToGoalCells(Grid, Goals[Index], static_cast<int>(Index), Cells))
		{
			GoalCells.push_back(Cells);
		}
	}

	if (GoalCells.empty())
		return false;

	const int2 Size = Grid.GetSize();
	const size_t CellCount = static_cast<size_t>(Size.x) * Size.y;
	if (Nodes.size() != CellCount)
	{
		Nodes.assign(CellCount, Node_s());
		Stamp = 0;
	}

	if (++Stamp == 0)
	{
		for (Node_s& Node : Nodes)
		{
			Node.Stamp = 0;
		}
		Stamp = 1;
	}

	auto ToIndex = [&](int2 Cell) { return Cell.y * Size.x + Cell.x; };
	auto Touch = [&](int Index) -> Node_s&
	{
		Node_s& Node = Nodes[Index];
		if (Node.Stamp != Stamp)
		{
			Node.G = FLT_MAX;
			Node.Parent = -1;
			Node.Stamp = Stamp;
			Node.Closed = false;
		}
		return Node;
	};

	auto ReachedGoal = [&](int2 Cell)
	{
		for (const GoalCells_s& Goal : GoalCells)
		{
			if (Cell.x >= Goal.Min.x && Cell.x <= Goal.Max.x && Cell.y >= Goal.Min.y && Cell.y <= Goal.Max.y)
				return Goal.GoalIndex;
		}
		return -1;
	};

	const int StartIndex = ToIndex(StartCell);

	const auto OpenGreater = [](const OpenEntry_s& A, const OpenEntry_s& B) { return A.F > B.F; };

	Open.clear();
	Touch(StartIndex).G = 0.0f;
	Open.push_back({ Heuristic(StartCell), 0.0f, StartIndex });

	static constexpr int2 Offsets[] = {
		int2(1, 0), int2(-1, 0), int2(0, 1), int2(0, -1),
		int2(1, 1), int2(1, -1), int2(-1, 1), int2(-1, -1),
	};

	int EndIndex = -1;
	while (!Open.empty())
	{
		std::pop_heap(Open.begin(), Open.end(), OpenGreater);
		const OpenEntry_s Entry = Open.back();
		Open.pop_back();

		Node_s& Current = Nodes[Entry.Index];
		if (Current.Closed || Entry.G > Current.G)
			continue;

		Current.Closed = true;

		const int2 Cell(Entry.Index % Size.x, Entry.Index / Size.x);

		OutGoalIndex = ReachedGoal(Cell);
		if (OutGoalIndex >= 0)
		{
			EndIndex = Entry.Index;
			break;
		}

		if (++LastExpansions > MaxExpansions)
			break;

		for (const int2& Offset : Offsets)
		{
			const int2 Next(Cell.x + Offset.x, Cell.y + Offset.y);
			if (!Grid.IsWalkable(Next))
				continue;

			const bool Diagonal = Offset.x != 0 && Offset.y != 0;
			if (Diagonal && (!Grid.IsWalkable(int2(Cell.x + Offset.x, Cell.y)) || !Grid.IsWalkable(int2(Cell.x, Cell.y + Offset.y))))
				continue;

			const int NextIndex = ToIndex(Next);
			Node_s& NextNode = Touch(NextIndex);
			if (NextNode.Closed)
				continue;

			const float NewG = Entry.G + (Diagonal ? Sqrt2 : 1.0f);
			if (NewG < NextNode.G)
			{
				NextNode.G = NewG;
				NextNode.Parent = Entry.Index;
				Open.push_back({ NewG + Heuristic(Next), NewG, NextIndex });
				std::push_heap(Open.begin(), Open.end(), OpenGreater);
			}
		}
	}

	if (EndIndex < 0)
		return false;

	for (int Index = EndIndex; Index != -1; Index = Nodes[Index].Parent)
	{
		RawPath.push_back(int2(Index % Size.x, Index / Size.x));
	}
	std::reverse(RawPath.begin(), RawPath.end());

	Smooth(Grid, OutPath);
	return true;
}

void NavPathfinder_c::Smooth(const NavGrid_c& Grid, std::vector<float3>& OutPath) const
{
	if (RawPath.empty())
		return;

	const size_t Last = RawPath.size() - 1;
	size_t Anchor = 0;
	OutPath.push_back(Grid.CellToWorld(RawPath[0]));

	while (Anchor < Last)
	{
		const float3 AnchorPosition = Grid.CellToWorld(RawPath[Anchor]);

		size_t Furthest = Anchor + 1;
		while (Furthest < Last && Grid.HasLineOfSight(AnchorPosition, Grid.CellToWorld(RawPath[Furthest + 1])))
		{
			++Furthest;
		}

		OutPath.push_back(Grid.CellToWorld(RawPath[Furthest]));
		Anchor = Furthest;
	}
}
