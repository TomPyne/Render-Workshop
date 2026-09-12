#include "RoadObject.h"
#include <Logging/Logging.h>

void RoadObject_c::OnConstruct()
{
	Super::OnConstruct();
}

RoadVertIndex_t RoadObject_c::GetHoveredVert() const
{
	return RoadVertIndex_t::INVALID;
}

RoadEdgeIndex_t RoadObject_c::GetHoveredEdge() const
{
	return RoadEdgeIndex_t::INVALID;
}


void RoadObject_c::BuildMesh()
{
}

void RoadVert_s::AddEdge(RoadEdgeIndex_t Edge)
{
	ASSERTMSG(NumEdges < 4, "AddEdge: Cannot add edge to vert, already has 4 edges");

	Edges[NumEdges] = Edge;
	NumEdges++;
}

bool RoadNetwork_s::IsValidVert(RoadVertIndex_t Vert) const
{
	if (Vert <= RoadVertIndex_t::INVALID)
		return false;

	if (Vertices.size() <= static_cast<size_t>(Vert))
	{
		LOGWARNING("IsValidVert: Invalid index %u, network has %zu vertices", Vert, Vertices.size());
		return false;
	}
	return true;
}

bool RoadNetwork_s::IsValidEdge(RoadEdgeIndex_t Edge) const
{
	if (Edge <= RoadEdgeIndex_t::INVALID)
		return false;

	if (Edges.size() <= static_cast<size_t>(Edge))
	{
		LOGWARNING("IsValidEdge: Invalid index %u, network has %zu edges", Edge, Edges.size());
		return false;
	}
	return true;
}

RoadEdgeIndex_t RoadNetwork_s::AddEdge(RoadVertIndex_t A, RoadVertIndex_t B)
{
	if (!IsValidVert(A) || !IsValidVert(B))
	{
		LOGWARNING("AddEdge: Invalid index");
		return RoadEdgeIndex_t::INVALID;
	}

	if (!GetVert(A).HasAvailableEdgeSlot() || !GetVert(B).HasAvailableEdgeSlot())
	{
		LOGWARNING("AddEdge: No available connection points");
		return RoadEdgeIndex_t::INVALID;
	}

	const RoadEdgeIndex_t Edge = CreateEdge(A, B);

	GetVert(A).AddEdge(Edge);
	GetVert(B).AddEdge(Edge);

	return Edge;
}

RoadVertIndex_t RoadNetwork_s::CreateVert(const float3& Position)
{
	const RoadVertIndex_t NewVert = static_cast<RoadVertIndex_t>(Vertices.size());
	Vertices.emplace_back(Position);
	return NewVert;
}

RoadEdgeIndex_t RoadNetwork_s::CreateEdge(RoadVertIndex_t A, RoadVertIndex_t B)
{
	const RoadEdgeIndex_t NewEdge = static_cast<RoadEdgeIndex_t>(Edges.size());
	Edges.emplace_back(A, B);
	return NewEdge;
}
