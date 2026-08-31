#pragma once

#include "Physics/IntersectionTypes.h"

class IPhysical_c;

// The query and its running best result, carried across every candidate in a scene trace.
// Path and the shape parameters are in query space.
struct IntersectionCtx_s
{
	static IntersectionCtx_s CreateLineTrace(float3 Start, float3 Direction, float MaxDistance, bool Complex = false)
	{
		IntersectionCtx_s Ctx = {};
		Ctx.Shape = TraceShape_e::Line;
		Ctx.Path.Start = Start;
		Ctx.Path.End = Start + Direction * MaxDistance;
		Ctx.Params.bTraceComplex = Complex;

		return Ctx;
	}

	static IntersectionCtx_s CreateLineSegmentTrace(float3 Start, float3 End, bool Complex = false)
	{
		IntersectionCtx_s Ctx = {};
		Ctx.Shape = TraceShape_e::Line;
		Ctx.Path.Start = Start;
		Ctx.Path.End = End;
		Ctx.Params.bTraceComplex = Complex;

		return Ctx;
	}

	Segment_s Path;
	TraceShape_e Shape = TraceShape_e::Line;

	// Sphere: Extents.x is the radius. Box: half widths, oriented by Axes. Line: unused.
	float3 Extents = {};
	float3 Axes[3] = {};

	TraceParams_s Params;

	void SetClosestHit(const IPhysical_c* Candidate, const Hit_s& InHit)
	{
		if (Closest == nullptr || InHit.T < Best.T)
		{
			Best = InHit;
			Closest = Candidate;
		}
	}

	LineTrace_s GetLineTrace() const { return LineTrace_s{ Path }; }

	bool HasHit() const { return Closest != nullptr; }
	float BestT() const { return Best.T; }
	const Hit_s& GetHit() const { return Best; }
	const IPhysical_c* GetClosest() const { return Closest; }

private:

	Hit_s Best = {};
	const IPhysical_c* Closest = nullptr;
};

class IPhysical_c
{
public:

	virtual ~IPhysical_c() = default;

	virtual void Intersect(IntersectionCtx_s& Context) const = 0;
};
