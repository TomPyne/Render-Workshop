#include "NavPathfinder.h"

#include "AI/NavGrid.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace
{
	constexpr float Sqrt2 = 1.41421356f;

	// Exact cost of an unobstructed 8-way path, in cells
	float OctileDistance(int2 A, int2 B)
	{
		const int DX = std::abs(A.x - B.x);
		const int DZ = std::abs(A.y - B.y);
		return static_cast<float>(DX + DZ) + (Sqrt2 - 2.0f) * static_cast<float>(Min(DX, DZ));
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

bool NavPathfinder_c::FindPath(const NavGrid_c& Grid, float3 Start, float3 Goal, std::vector<float3>& OutPath)
{
	OutPath.clear();
	RawPath.clear();
	LastExpansions = 0;

	int2 StartCell;
	int2 GoalCell;
	if (!SnapToCell(Grid, Start, StartCell) || !SnapToCell(Grid, Goal, GoalCell))
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

	const int StartIndex = ToIndex(StartCell);
	const int GoalIndex = ToIndex(GoalCell);

	const auto OpenGreater = [](const OpenEntry_s& A, const OpenEntry_s& B) { return A.F > B.F; };

	Open.clear();
	Touch(StartIndex).G = 0.0f;
	Open.push_back({ OctileDistance(StartCell, GoalCell), 0.0f, StartIndex });

	static constexpr int2 Offsets[] = {
		int2(1, 0), int2(-1, 0), int2(0, 1), int2(0, -1),
		int2(1, 1), int2(1, -1), int2(-1, 1), int2(-1, -1),
	};

	bool Found = false;
	while (!Open.empty())
	{
		std::pop_heap(Open.begin(), Open.end(), OpenGreater);
		const OpenEntry_s Entry = Open.back();
		Open.pop_back();

		Node_s& Current = Nodes[Entry.Index];
		if (Current.Closed || Entry.G > Current.G)
			continue;

		Current.Closed = true;

		if (Entry.Index == GoalIndex)
		{
			Found = true;
			break;
		}

		if (++LastExpansions > MaxExpansions)
			break;

		const int2 Cell(Entry.Index % Size.x, Entry.Index / Size.x);
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
				Open.push_back({ NewG + OctileDistance(Next, GoalCell), NewG, NextIndex });
				std::push_heap(Open.begin(), Open.end(), OpenGreater);
			}
		}
	}

	if (!Found)
		return false;

	for (int Index = GoalIndex; Index != -1; Index = Nodes[Index].Parent)
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
