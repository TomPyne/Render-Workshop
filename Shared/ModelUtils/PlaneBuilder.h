#pragma once

#include <SurfMath.h>

#include <cstdint>
#include <vector>

namespace PlaneBuilder
{
	template<typename IndexType>
	struct PlaneMesh_s
	{
		std::vector<float3> Positions;
		std::vector<IndexType> Indices;
	};

	PlaneMesh_s<uint32_t> BuildPlaneMesh32(float2 Scale);
	PlaneMesh_s<uint16_t> BuildPlaneMesh16(float2 Scale);
}