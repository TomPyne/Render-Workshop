#pragma once

#include "Physics/IntersectionTypes.h"

struct Mesh_s;

// Both directions of a candidate's transform, plus the scale information the sweep tests need.
// Build one per candidate, never per surface or per triangle, since inverting is not cheap.
struct CandidateSpace_s
{
	matrix ToQuery = MakeMatrixIdentity();
	matrix ToCandidate = MakeMatrixIdentity();
	matrix NormalToQuery = MakeMatrixIdentity();

	float MaxAxisScale = 1.0f;
	float MinAxisScale = 1.0f;
	bool UniformScale = true;
};

CandidateSpace_s MakeCandidateSpace(const matrix& CandidateToQuery) noexcept;

Segment_s TransformSegment(const Segment_s& Segment, const matrix& Transform) noexcept;

// Trace is in query space, Mesh is in candidate space, Space carries the mapping between them.
//
// InOutHit is in/out: seed T with the best result found so far and the bounds rejection can skip
// this candidate entirely. Only written on a closer hit, so a false return leaves it untouched.
bool TraceLineMesh(const LineTrace_s& Trace, const Mesh_s& Mesh, const CandidateSpace_s& Space, const TraceParams_s& Params, Hit_s& InOutHit);
