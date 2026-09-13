#include "Physics/Intersection.h"

#include "Physics/IntersectionKernels.h"
#include "Rendering/Mesh.h"

CandidateSpace_s MakeCandidateSpace(const matrix& CandidateToQuery) noexcept
{
	CandidateSpace_s Space;
	Space.ToQuery = CandidateToQuery;

	float Determinant = 0.0f;
	Space.ToCandidate = InverseMatrix(CandidateToQuery, &Determinant);

	// A zero scale on any axis collapses the transform, and InverseMatrix divides through by the
	// determinant without guarding, so everything derived from it below would be NaN.
	Space.Valid = fabsf(Determinant) > 1e-12f;
	if (!Space.Valid)
	{
		return Space;
	}

	// A mirror maps a front facing triangle onto a back facing one, since the winding normal
	// transforms as determinant * the inverse transpose rather than the inverse transpose alone.
	Space.WindingSign = Determinant < 0.0f ? -1.0f : 1.0f;

	// The inverse transpose of ToQuery. Transposing an affine matrix leaves r[3] as (0, 0, 0, 1),
	// so TransformF3 applies no translation and this can be used on a normal directly.
	Space.NormalToQuery = TransposeMatrix(Space.ToCandidate);

	const float ScaleX = Length(CandidateToQuery.r[0].xyz);
	const float ScaleY = Length(CandidateToQuery.r[1].xyz);
	const float ScaleZ = Length(CandidateToQuery.r[2].xyz);

	Space.MaxAxisScale = Max(ScaleX, Max(ScaleY, ScaleZ));
	Space.MinAxisScale = Min(ScaleX, Min(ScaleY, ScaleZ));
	Space.UniformScale = (Space.MaxAxisScale - Space.MinAxisScale) <= (1e-4f * Space.MaxAxisScale);

	return Space;
}

Segment_s TransformSegment(const Segment_s& Segment, const matrix& Transform) noexcept
{
	Segment_s Result;
	Result.Start = TransformF3(Segment.Start, Transform);
	Result.End = TransformF3(Segment.End, Transform);

	return Result;
}

bool TraceLineMesh(const LineTrace_s& Trace, const Mesh_s& Mesh, const CandidateSpace_s& Space, const TraceParams_s& Params, Hit_s& InOutHit)
{
	if (!Mesh.Ready || !Space.Valid)
	{
		return false;
	}

	const Segment_s LocalPath = TransformSegment(Trace.Path, Space.ToCandidate);
	const SegmentCache_s LocalCache = MakeSegmentCache(LocalPath);

	float BoundsEntry = 0.0f;
	float BoundsExit = 0.0f;
	if (!LineAABB(LocalCache, Mesh.Bounds, BoundsEntry, BoundsExit) || BoundsEntry >= InOutHit.T)
	{
		return false;
	}

	Hit_s LocalHit;
	LocalHit.T = InOutHit.T;

	bool bHit = false;

	if (!Params.bTraceComplex)
	{
		LocalHit.T = BoundsEntry;
		LocalHit.Initial = BoundsEntry <= 0.0f;
		LocalHit.Normal = AABBEntryNormal(LocalCache, Mesh.Bounds);

		bHit = true;
	}
	else
	{
		for (const Surface_s& Surface : Mesh.Surfaces)
		{
			float SurfaceEntry = 0.0f;
			float SurfaceExit = 0.0f;
			if (!LineAABB(LocalCache, Surface.Bounds, SurfaceEntry, SurfaceExit) || SurfaceEntry >= LocalHit.T)
			{
				continue;
			}

			const u32 IndexEnd = Surface.IndexOffset + Surface.IndexCount;
			for (u32 IndexIt = Surface.IndexOffset; IndexIt + 2 < IndexEnd; IndexIt += 3)
			{
				Triangle_s Triangle;
				Triangle.V0 = Mesh.Vertices[Mesh.Indices[IndexIt + 0]];
				Triangle.V1 = Mesh.Vertices[Mesh.Indices[IndexIt + 1]];
				Triangle.V2 = Mesh.Vertices[Mesh.Indices[IndexIt + 2]];

				float T = 0.0f;
				float3 Barycentric = {};
				if (!LineTriangle(LocalPath, Triangle, Params.bAllowBackFaces, Space.WindingSign, Params.Epsilon, T, Barycentric) || T >= LocalHit.T)
				{
					continue;
				}

				LocalHit.T = T;
				LocalHit.Index = IndexIt / 3;
				LocalHit.Normal = Cross(Triangle.Edge1(), Triangle.Edge2());
				LocalHit.Initial = false;

				bHit = true;
			}
		}
	}

	if (!bHit)
	{
		return false;
	}

	InOutHit.T = LocalHit.T;
	InOutHit.Index = LocalHit.Index;
	InOutHit.Initial = LocalHit.Initial;

	// t is unchanged by the transform, so the point is taken along the query space path rather
	// than transformed back out of candidate space.
	InOutHit.Location = Trace.Path.PointAt(LocalHit.T);

	if (LocalHit.Initial)
	{
		// Nothing was crossed, so the only direction worth reporting is back along the trace.
		InOutHit.Normal = NegateF3(Normalize(Trace.Path.Delta()));

		return true;
	}

	InOutHit.Normal = Normalize(TransformF3(LocalHit.Normal, Space.NormalToQuery));

	if (Dot(InOutHit.Normal, Trace.Path.Delta()) > 0.0f)
	{
		InOutHit.Normal = NegateF3(InOutHit.Normal);
	}

	return true;
}
