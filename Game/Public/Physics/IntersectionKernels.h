#pragma once

#include "Physics/IntersectionTypes.h"

// Kernels take both operands in the same space and never see a matrix. The space change lives one
// layer up, in Intersection.h.

// Slab test. OutTEntry and OutTExit are clamped to the segment, so a trace starting inside the
// bounds reports an entry of 0.
inline bool LineAABB(const SegmentCache_s& Segment, const AABB& Bounds, float& OutTEntry, float& OutTExit) noexcept
{
	const float3 T0 = (Bounds.mins - Segment.Start) * Segment.InvDelta;
	const float3 T1 = (Bounds.maxs - Segment.Start) * Segment.InvDelta;

	const float3 Entry = MinVector(T0, T1);
	const float3 Exit = MaxVector(T0, T1);

	OutTEntry = Max(Max(Entry.x, Entry.y), Max(Entry.z, 0.0f));
	OutTExit = Min(Min(Exit.x, Exit.y), Min(Exit.z, 1.0f));

	return OutTEntry <= OutTExit;
}

// Outward face normal of the slab that produced the entry time. Only meaningful when LineAABB
// returned an entry greater than zero.
inline float3 AABBEntryNormal(const SegmentCache_s& Segment, const AABB& Bounds) noexcept
{
	const float3 T0 = (Bounds.mins - Segment.Start) * Segment.InvDelta;
	const float3 T1 = (Bounds.maxs - Segment.Start) * Segment.InvDelta;
	const float3 Entry = MinVector(T0, T1);

	if (Entry.x >= Entry.y && Entry.x >= Entry.z)
	{
		return float3(Segment.Delta.x > 0.0f ? -1.0f : 1.0f, 0.0f, 0.0f);
	}

	if (Entry.y >= Entry.z)
	{
		return float3(0.0f, Segment.Delta.y > 0.0f ? -1.0f : 1.0f, 0.0f);
	}

	return float3(0.0f, 0.0f, Segment.Delta.z > 0.0f ? -1.0f : 1.0f);
}

// Moller-Trumbore. OutBarycentric is (W, U, V) weighting (V0, V1, V2).
inline bool LineTriangle(const Segment_s& Segment, const Triangle_s& Triangle, bool bAllowBackFaces, float WindingSign, float Epsilon, float& OutT, float3& OutBarycentric) noexcept
{
	const float3 Edge1 = Triangle.Edge1();
	const float3 Edge2 = Triangle.Edge2();
	const float3 Delta = Segment.Delta();

	const float3 P = Cross(Delta, Edge2);
	const float Determinant = Dot(Edge1, P);

	// Determinant is -Dot(Delta, Cross(Edge1, Edge2)), so it is positive when the trace runs
	// against the geometric normal, which is a front facing hit for clockwise winding. WindingSign
	// is -1 when the candidate transform mirrors, which reverses which side counts as the front.
	if (bAllowBackFaces ? fabsf(Determinant) < Epsilon : (Determinant * WindingSign) < Epsilon)
	{
		return false;
	}

	const float InvDeterminant = 1.0f / Determinant;
	const float3 ToV0 = Segment.Start - Triangle.V0;

	const float U = Dot(ToV0, P) * InvDeterminant;
	if (U < 0.0f || U > 1.0f)
	{
		return false;
	}

	const float3 Q = Cross(ToV0, Edge1);

	const float V = Dot(Delta, Q) * InvDeterminant;
	if (V < 0.0f || U + V > 1.0f)
	{
		return false;
	}

	const float T = Dot(Edge2, Q) * InvDeterminant;
	if (T < 0.0f || T > 1.0f)
	{
		return false;
	}

	OutT = T;
	OutBarycentric = float3(1.0f - U - V, U, V);

	return true;
}
