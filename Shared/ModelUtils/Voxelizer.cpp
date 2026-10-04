#include "Voxelizer.h"

#include "Jobs/JobSystem.h"

#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>

namespace
{

constexpr uint32_t kSignRayCount = 16;
// A voxel is inside when more than this many sign rays first hit a back face.
constexpr uint32_t kInsideBackFaceHits = kSignRayCount / 4;

constexpr uint32_t kBVHLeafSize = 4;
constexpr uint32_t kBVHStackSize = 64;

// Squared length of Cross(Edge1, Edge2) below which a triangle is dropped as degenerate.
constexpr float kDegenerateCrossSqr = 1e-12f;
constexpr float kRayDeterminantEpsilon = 1e-12f;

struct VoxelTriangle_s
{
	float3 V0;
	float3 V1;
	float3 V2;
};

struct BVHNode_s
{
	AABB Bounds = {};
	// First triangle for a leaf, left child for an interior node, whose right child is First + 1.
	uint32_t First = 0;
	// Zero for an interior node.
	uint32_t Count = 0;
};

struct TriangleBVH_s
{
	std::vector<VoxelTriangle_s> Triangles;
	std::vector<BVHNode_s> Nodes;
};

struct SignRay_s
{
	float3 Direction;
	float3 InvDirection;
};

void BuildNode(TriangleBVH_s& BVH, const std::vector<float3>& Centroids, std::vector<uint32_t>& Order, uint32_t NodeIndex, uint32_t First, uint32_t Count)
{
	AABB Bounds;
	AABB CentroidBounds;
	for (uint32_t It = First; It < First + Count; It++)
	{
		const VoxelTriangle_s& Triangle = BVH.Triangles[Order[It]];
		Bounds.Grow(Triangle.V0);
		Bounds.Grow(Triangle.V1);
		Bounds.Grow(Triangle.V2);
		CentroidBounds.Grow(Centroids[Order[It]]);
	}

	BVH.Nodes[NodeIndex].Bounds = Bounds;
	BVH.Nodes[NodeIndex].First = First;
	BVH.Nodes[NodeIndex].Count = Count;

	if (Count <= kBVHLeafSize)
	{
		return;
	}

	const float3 CentroidExtent = CentroidBounds.maxs - CentroidBounds.mins;
	const uint32_t Axis = (CentroidExtent.x >= CentroidExtent.y && CentroidExtent.x >= CentroidExtent.z) ? 0 : (CentroidExtent.y >= CentroidExtent.z ? 1 : 2);
	if (CentroidExtent.v[Axis] <= 0.0f)
	{
		return;
	}

	const uint32_t LeftCount = Count / 2;
	std::nth_element(Order.begin() + First, Order.begin() + First + LeftCount, Order.begin() + First + Count, [&](uint32_t A, uint32_t B)
	{
		return Centroids[A].v[Axis] < Centroids[B].v[Axis];
	});

	const uint32_t LeftIndex = static_cast<uint32_t>(BVH.Nodes.size());
	BVH.Nodes.emplace_back();
	BVH.Nodes.emplace_back();

	BVH.Nodes[NodeIndex].First = LeftIndex;
	BVH.Nodes[NodeIndex].Count = 0;

	BuildNode(BVH, Centroids, Order, LeftIndex, First, LeftCount);
	BuildNode(BVH, Centroids, Order, LeftIndex + 1, First + LeftCount, Count - LeftCount);
}

void BuildBVH(TriangleBVH_s& BVH)
{
	const uint32_t TriangleCount = static_cast<uint32_t>(BVH.Triangles.size());

	std::vector<float3> Centroids(TriangleCount);
	std::vector<uint32_t> Order(TriangleCount);
	for (uint32_t It = 0; It < TriangleCount; It++)
	{
		const VoxelTriangle_s& Triangle = BVH.Triangles[It];
		Centroids[It] = (Triangle.V0 + Triangle.V1 + Triangle.V2) * (1.0f / 3.0f);
		Order[It] = It;
	}

	BVH.Nodes.reserve(TriangleCount * 2);
	BVH.Nodes.emplace_back();
	BuildNode(BVH, Centroids, Order, 0, 0, TriangleCount);

	// Leaves address contiguous ranges, so lay the triangles out in build order.
	std::vector<VoxelTriangle_s> Sorted(TriangleCount);
	for (uint32_t It = 0; It < TriangleCount; It++)
	{
		Sorted[It] = BVH.Triangles[Order[It]];
	}
	BVH.Triangles = std::move(Sorted);
}

float PointAABBDistSqr(const float3& Point, const AABB& Bounds)
{
	const float3 Outside = MaxVector(MaxVector(Bounds.mins - Point, Point - Bounds.maxs), float3(0.0f));
	return LengthSqr(Outside);
}

// Ericson, Real-Time Collision Detection 5.1.5.
float3 ClosestPointOnTriangle(const float3& Point, const VoxelTriangle_s& Triangle)
{
	const float3 AB = Triangle.V1 - Triangle.V0;
	const float3 AC = Triangle.V2 - Triangle.V0;

	const float3 AP = Point - Triangle.V0;
	const float D1 = Dot(AB, AP);
	const float D2 = Dot(AC, AP);
	if (D1 <= 0.0f && D2 <= 0.0f)
	{
		return Triangle.V0;
	}

	const float3 BP = Point - Triangle.V1;
	const float D3 = Dot(AB, BP);
	const float D4 = Dot(AC, BP);
	if (D3 >= 0.0f && D4 <= D3)
	{
		return Triangle.V1;
	}

	const float VC = D1 * D4 - D3 * D2;
	if (VC <= 0.0f && D1 >= 0.0f && D3 <= 0.0f)
	{
		return Triangle.V0 + AB * (D1 / (D1 - D3));
	}

	const float3 CP = Point - Triangle.V2;
	const float D5 = Dot(AB, CP);
	const float D6 = Dot(AC, CP);
	if (D6 >= 0.0f && D5 <= D6)
	{
		return Triangle.V2;
	}

	const float VB = D5 * D2 - D1 * D6;
	if (VB <= 0.0f && D2 >= 0.0f && D6 <= 0.0f)
	{
		return Triangle.V0 + AC * (D2 / (D2 - D6));
	}

	const float VA = D3 * D6 - D5 * D4;
	if (VA <= 0.0f && (D4 - D3) >= 0.0f && (D5 - D6) >= 0.0f)
	{
		return Triangle.V1 + (Triangle.V2 - Triangle.V1) * ((D4 - D3) / ((D4 - D3) + (D5 - D6)));
	}

	const float InvDenominator = 1.0f / (VA + VB + VC);
	return Triangle.V0 + AB * (VB * InvDenominator) + AC * (VC * InvDenominator);
}

float ClosestDistanceSqr(const TriangleBVH_s& BVH, const float3& Point)
{
	float Best = FLT_MAX;

	uint32_t Stack[kBVHStackSize];
	uint32_t StackSize = 0;
	Stack[StackSize++] = 0;

	while (StackSize > 0)
	{
		const BVHNode_s& Node = BVH.Nodes[Stack[--StackSize]];
		if (PointAABBDistSqr(Point, Node.Bounds) >= Best)
		{
			continue;
		}

		if (Node.Count > 0)
		{
			for (uint32_t It = Node.First; It < Node.First + Node.Count; It++)
			{
				Best = Min(Best, DistSqr(Point, ClosestPointOnTriangle(Point, BVH.Triangles[It])));
			}
			continue;
		}

		// Push the farther child first, so the nearer one tightens Best before the other is tested.
		const float LeftDistSqr = PointAABBDistSqr(Point, BVH.Nodes[Node.First].Bounds);
		const float RightDistSqr = PointAABBDistSqr(Point, BVH.Nodes[Node.First + 1].Bounds);
		Stack[StackSize++] = LeftDistSqr < RightDistSqr ? Node.First + 1 : Node.First;
		Stack[StackSize++] = LeftDistSqr < RightDistSqr ? Node.First : Node.First + 1;
	}

	return Best;
}

bool RayAABB(const float3& Origin, const float3& InvDirection, const AABB& Bounds, float MaxT)
{
	const float3 T0 = (Bounds.mins - Origin) * InvDirection;
	const float3 T1 = (Bounds.maxs - Origin) * InvDirection;

	const float3 Entry = MinVector(T0, T1);
	const float3 Exit = MaxVector(T0, T1);

	const float TEntry = Max(Max(Entry.x, Entry.y), Max(Entry.z, 0.0f));
	const float TExit = Min(Min(Exit.x, Exit.y), Min(Exit.z, MaxT));

	return TEntry <= TExit;
}

// Two sided Moller-Trumbore. OutDeterminant is -Dot(Direction, Cross(Edge1, Edge2)), positive for a
// front face hit, matching LineTriangle.
bool RayTriangle(const float3& Origin, const float3& Direction, const VoxelTriangle_s& Triangle, float& OutT, float& OutDeterminant)
{
	const float3 Edge1 = Triangle.V1 - Triangle.V0;
	const float3 Edge2 = Triangle.V2 - Triangle.V0;

	const float3 P = Cross(Direction, Edge2);
	const float Determinant = Dot(Edge1, P);
	if (fabsf(Determinant) < kRayDeterminantEpsilon)
	{
		return false;
	}

	const float InvDeterminant = 1.0f / Determinant;
	const float3 ToV0 = Origin - Triangle.V0;

	const float U = Dot(ToV0, P) * InvDeterminant;
	if (U < 0.0f || U > 1.0f)
	{
		return false;
	}

	const float3 Q = Cross(ToV0, Edge1);

	const float V = Dot(Direction, Q) * InvDeterminant;
	if (V < 0.0f || U + V > 1.0f)
	{
		return false;
	}

	const float T = Dot(Edge2, Q) * InvDeterminant;
	if (T <= 0.0f)
	{
		return false;
	}

	OutT = T;
	OutDeterminant = Determinant;

	return true;
}

bool FirstHitIsBackFace(const TriangleBVH_s& BVH, const float3& Origin, const SignRay_s& Ray)
{
	float BestT = FLT_MAX;
	float BestDeterminant = 0.0f;

	uint32_t Stack[kBVHStackSize];
	uint32_t StackSize = 0;
	Stack[StackSize++] = 0;

	while (StackSize > 0)
	{
		const BVHNode_s& Node = BVH.Nodes[Stack[--StackSize]];
		if (!RayAABB(Origin, Ray.InvDirection, Node.Bounds, BestT))
		{
			continue;
		}

		if (Node.Count > 0)
		{
			for (uint32_t It = Node.First; It < Node.First + Node.Count; It++)
			{
				float T;
				float Determinant;
				if (RayTriangle(Origin, Ray.Direction, BVH.Triangles[It], T, Determinant) && T < BestT)
				{
					BestT = T;
					BestDeterminant = Determinant;
				}
			}
			continue;
		}

		Stack[StackSize++] = Node.First + 1;
		Stack[StackSize++] = Node.First;
	}

	return BestDeterminant < 0.0f;
}

// Fibonacci sphere, so the rays cover every direction evenly.
std::array<SignRay_s, kSignRayCount> MakeSignRays()
{
	constexpr float kGoldenAngle = 2.39996323f;
	// Keeps the reciprocal finite, so the slab test never multiplies zero by infinity.
	constexpr float kMinComponent = 1e-6f;

	std::array<SignRay_s, kSignRayCount> Rays;
	for (uint32_t It = 0; It < kSignRayCount; It++)
	{
		const float Y = 1.0f - (2.0f * It + 1.0f) / kSignRayCount;
		const float Radius = sqrtf(1.0f - Y * Y);
		const float Phi = kGoldenAngle * It;

		Rays[It].Direction = float3(Radius * cosf(Phi), Y, Radius * sinf(Phi));
		for (uint32_t Axis = 0; Axis < 3; Axis++)
		{
			const float Component = Rays[It].Direction.v[Axis];
			Rays[It].InvDirection.v[Axis] = 1.0f / (fabsf(Component) < kMinComponent ? copysignf(kMinComponent, Component) : Component);
		}
	}

	return Rays;
}

void VoxelizeVoxel(const TriangleBVH_s& BVH, const std::array<SignRay_s, kSignRayCount>& SignRays, const float3& Point, float& OutDistance)
{
	const float Distance = sqrtf(ClosestDistanceSqr(BVH, Point));

	uint32_t BackFaceHits = 0;
	for (uint32_t RayIt = 0; RayIt < kSignRayCount; RayIt++)
	{
		// Stop once the remaining rays can no longer change the outcome.
		if (BackFaceHits > kInsideBackFaceHits || BackFaceHits + (kSignRayCount - RayIt) <= kInsideBackFaceHits)
		{
			break;
		}

		if (FirstHitIsBackFace(BVH, Point, SignRays[RayIt]))
		{
			BackFaceHits++;
		}
	}

	OutDistance = BackFaceHits > kInsideBackFaceHits ? -Distance : Distance;
}

}

bool VoxelizeGeometry(const VoxelizerInput_s& Input, VoxelizerOutput_s& Output)
{
	Output = {};

	if (!Input.Positions || !Input.Indices || Input.IndexCount < 3 || Input.IndexCount % 3 != 0)
	{
		return false;
	}

	if (Input.Resolution <= Input.PaddingVoxels * 2)
	{
		return false;
	}

	TriangleBVH_s BVH;
	BVH.Triangles.reserve(Input.IndexCount / 3);

	AABB MeshBounds;
	for (uint32_t It = 0; It < Input.IndexCount; It += 3)
	{
		const uint32_t I0 = Input.Indices[It + 0];
		const uint32_t I1 = Input.Indices[It + 1];
		const uint32_t I2 = Input.Indices[It + 2];
		if (I0 >= Input.VertexCount || I1 >= Input.VertexCount || I2 >= Input.VertexCount)
		{
			return false;
		}

		const VoxelTriangle_s Triangle = { Input.Positions[I0], Input.Positions[I1], Input.Positions[I2] };
		if (LengthSqr(Cross(Triangle.V1 - Triangle.V0, Triangle.V2 - Triangle.V0)) < kDegenerateCrossSqr)
		{
			continue;
		}

		BVH.Triangles.push_back(Triangle);
		MeshBounds.Grow(Triangle.V0);
		MeshBounds.Grow(Triangle.V1);
		MeshBounds.Grow(Triangle.V2);
	}

	if (BVH.Triangles.empty())
	{
		return false;
	}

	BuildBVH(BVH);

	const float3 Extent = MeshBounds.maxs - MeshBounds.mins;
	const uint32_t InnerResolution = Input.Resolution - Input.PaddingVoxels * 2;
	const float VoxelSize = Max(Max(Extent.x, Extent.y), Extent.z) / InnerResolution;

	uint3 Dims;
	for (uint32_t Axis = 0; Axis < 3; Axis++)
	{
		// The bias stops float error rounding the longest axis up past InnerResolution.
		const uint32_t InnerVoxels = static_cast<uint32_t>(ceilf(Extent.v[Axis] / VoxelSize - 1e-3f));
		Dims.v[Axis] = Clamp(InnerVoxels, 1u, InnerResolution) + Input.PaddingVoxels * 2;
	}

	const float3 Centre = MeshBounds.Origin();
	const float3 HalfSize = float3(Dims) * (VoxelSize * 0.5f);

	Output.Dims = Dims;
	Output.VolumeBounds = AABB(Centre - HalfSize, Centre + HalfSize);
	Output.VoxelSize = VoxelSize;
	Output.Distances.resize(static_cast<size_t>(Dims.x) * Dims.y * Dims.z);

	const std::array<SignRay_s, kSignRayCount> SignRays = MakeSignRays();

	const auto VoxelizeSlices = [&BVH, &SignRays, &Output](uint32_t BeginZ, uint32_t EndZ)
	{
		const uint3 Dims = Output.Dims;
		const float VoxelSize = Output.VoxelSize;

		size_t VoxelIndex = static_cast<size_t>(BeginZ) * Dims.x * Dims.y;
		for (uint32_t Z = BeginZ; Z < EndZ; Z++)
		{
			for (uint32_t Y = 0; Y < Dims.y; Y++)
			{
				for (uint32_t X = 0; X < Dims.x; X++)
				{
					VoxelizeVoxel(BVH, SignRays, Output.VolumeBounds.mins + (float3(X, Y, Z) + 0.5f) * VoxelSize, Output.Distances[VoxelIndex++]);
				}
			}
		}
	};

	if (Input.Parallel)
	{
		JobWaitOrHelp(JobParallelFor(Dims.z, 1, VoxelizeSlices, "VoxelizeGeometry"));
	}
	else
	{
		VoxelizeSlices(0, Dims.z);
	}

	return true;
}

std::vector<uint8_t> QuantizeDistances(const std::vector<float>& Distances, float MaxDistance)
{
	assert(MaxDistance > 0.0f);

	const float Scale = 127.0f / MaxDistance;

	std::vector<uint8_t> Quantized(Distances.size());
	for (size_t It = 0; It < Distances.size(); It++)
	{
		Quantized[It] = static_cast<uint8_t>(lroundf(128.0f + Clamp(Distances[It] * Scale, -127.0f, 127.0f)));
	}

	return Quantized;
}
