#pragma once

#include <SurfMath.h>

// Traces are parameterised as P(t) = Start + t * Delta, with t in [0, 1].
//
// For an affine M, M(Start + t * Delta) == M(Start) + t * (M3x3 * Delta), so t is identical in
// every space. Hits found in a candidate's local space therefore compare directly against hits
// from candidates with completely different transforms, with no conversion.
struct Segment_s
{
	float3 Start = {};
	float3 End = {};

	float3 Delta() const noexcept { return End - Start; }
	float3 PointAt(float T) const noexcept { return MultiplyAdd(Delta(), float3(T), Start); }
};

struct SegmentCache_s
{
	float3 Start = {};
	float3 Delta = {};
	float3 InvDelta = {};

	float3 PointAt(float T) const noexcept { return MultiplyAdd(Delta, float3(T), Start); }
};

struct Triangle_s
{
	float3 V0 = {};
	float3 V1 = {};
	float3 V2 = {};

	float3 Edge1() const noexcept { return V1 - V0; }
	float3 Edge2() const noexcept { return V2 - V0; }
};

struct Sphere_s
{
	float3 Centre = {};
	float Radius = 0.0f;
};

// Axes are unit length and orthonormal, Extents are half widths.
struct OBB_s
{
	float3 Centre = {};
	float3 Extents = {};
	float3 Axes[3] = {};
};

enum class TraceShape_e : u8
{
	Line,
	Sphere,
	Box,
};

struct LineTrace_s
{
	Segment_s Path;
};

struct SphereTrace_s
{
	Segment_s Path;
	float Radius = 0.0f;
};

struct BoxTrace_s
{
	Segment_s Path;
	float3 Extents = {};
	float3 Axes[3] = {};
};

struct Hit_s
{
	float T = 1.0f;
	float3 Location = {};
	float3 Normal = {};
	u32 Index = ~0u;

	// The trace began already overlapping. Normal is a depenetration direction, not a surface normal.
	bool Initial = false;
};

struct TraceParams_s
{
	bool bTraceComplex = true;
	bool bAllowBackFaces = false;

	// Rejects triangles whose Moller-Trumbore determinant is degenerate. The determinant scales
	// with edge length squared times trace length, so this is not scale invariant.
	float Epsilon = 1e-6f;
};

// 1/V, clamped to a finite magnitude. An infinite reciprocal times a zero slab distance is NaN,
// which is exactly the case of a trace lying in the plane of a slab.
inline float SafeInverse(float V) noexcept
{
	const float Inverse = 1.0f / V;
	if (ISINF(Inverse))
	{
		return Inverse > 0.0f ? FLT_MAX : -FLT_MAX;
	}

	return Inverse;
}

inline SegmentCache_s MakeSegmentCache(const Segment_s& Segment) noexcept
{
	SegmentCache_s Cache;
	Cache.Start = Segment.Start;
	Cache.Delta = Segment.Delta();
	Cache.InvDelta = float3(SafeInverse(Cache.Delta.x), SafeInverse(Cache.Delta.y), SafeInverse(Cache.Delta.z));

	return Cache;
}
