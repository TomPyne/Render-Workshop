#pragma once

#include <SurfMath.h>

#include <cstdint>
#include <vector>

struct TangentSpaceStream_s
{
	const void* Data = nullptr;
	uint32_t Stride = 0;
};

struct TangentSpaceInput_s
{
	TangentSpaceStream_s Positions;
	TangentSpaceStream_s Normals;
	TangentSpaceStream_s Texcoords;
	uint32_t VertexCount = 0;

	const uint32_t* Indices = nullptr;
	uint32_t IndexCount = 0;
};

struct TangentSpaceOutput_s
{
	std::vector<float3> Positions;
	std::vector<float3> Normals;
	std::vector<float2> Texcoords;
	std::vector<float4> Tangents;
	std::vector<uint32_t> Indices;
};

// Generates per-vertex tangents with MikkTSpace, splitting source vertices where the tangent frame
// is discontinuous. Face order and index count are preserved, so surface index ranges stay valid.
bool GenerateTangents(const TangentSpaceInput_s& Input, TangentSpaceOutput_s& Output);
