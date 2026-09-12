#pragma once

#include <Object/SpatialObject.h>

#include <memory>
#include <vector>

struct Mesh_s;
class MeshComponent_c;

enum class RoadVertIndex_t : int32_t { INVALID };
enum class RoadEdgeIndex_t : int32_t { INVALID };

struct RoadVert_s
{
	RoadVert_s(const float3& InPosition)
		: Position (InPosition)
		{}
	float3 Position;
	uint8_t NumEdges = 0;
	RoadEdgeIndex_t Edges[4] = { RoadEdgeIndex_t::INVALID };

	void AddEdge(RoadEdgeIndex_t Edge);
	bool HasAvailableEdgeSlot() const { return NumEdges < 4; }
};

struct RoadEdge_s
{
	RoadVertIndex_t VertA = RoadVertIndex_t::INVALID;
	RoadVertIndex_t VertB = RoadVertIndex_t::INVALID;
};

struct RoadNetwork_s
{
	bool IsValidVert(RoadVertIndex_t Vert) const;
	bool IsValidEdge(RoadEdgeIndex_t Edge) const;

	RoadVertIndex_t AddVert(const float3& Position);
	RoadEdgeIndex_t AddEdge(RoadVertIndex_t A, RoadVertIndex_t B);

	bool RemoveEdge(RoadEdge_s Edge);
	bool RemoveVert(RoadVertIndex_t Vert);

private:

	RoadVertIndex_t CreateVert(const float3& Position);
	RoadEdgeIndex_t CreateEdge(RoadVertIndex_t A, RoadVertIndex_t B);

	RoadVert_s& GetVert(RoadVertIndex_t Vert) { return Vertices[static_cast<size_t>(Vert)]; }
	RoadEdge_s& GetEdge(RoadEdgeIndex_t Edge) { return Edges[static_cast<size_t>(Edge)]; }

	std::vector<RoadVert_s> Vertices;
	std::vector<RoadEdge_s> Edges;
};

class RoadObject_c : public SpatialObject_c
{
	OBJECT_BODY(RoadObject_c, SpatialObject_c)

	// Begin Object_c interface
	virtual void OnConstruct() override;
	// End Object_c interface

	RoadVertIndex_t GetHoveredVert() const;
	RoadEdgeIndex_t GetHoveredEdge() const;



	void BuildMesh();

protected:

	RoadNetwork_s Network;
};