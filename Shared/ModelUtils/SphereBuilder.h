#pragma once

#include <SurfMath.h>

#include <cstdint>
#include <vector>

namespace SphereBuilder
{
	enum class SphereType_e
	{
		UVSPHERE,
		COUNT,
	};

	struct SphereMesh_s
	{
		std::vector<float3> Positions;
		std::vector<uint16_t> Indices;
	};

	struct SphereMeshDesc_s
	{
		SphereType_e Type = SphereType_e::UVSPHERE;
		uint32_t LatitudeSegments = 16;
		uint32_t LongitudeSegments = 16;
		float Radius = 1.0f;
	};

	SphereMesh_s BuildSphereMesh(const SphereMeshDesc_s& Desc);
}