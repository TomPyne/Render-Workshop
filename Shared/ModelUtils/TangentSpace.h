#pragma once

#include <SurfMath.h>

#include <cstdint>
#include <vector>

struct TangentSpaceStream_s
{
	const void* Data = nullptr;
	uint32_t Stride = 0;
};

constexpr uint32_t kTangentSpaceMaxTexcoords = 4;

struct TangentSpaceInput_s
{
	TangentSpaceStream_s Positions;
	TangentSpaceStream_s Normals;
	// Texcoords[0] builds the tangent basis, the remaining channels are carried through the split.
	TangentSpaceStream_s Texcoords[kTangentSpaceMaxTexcoords];
	uint32_t TexcoordCount = 0;
	uint32_t VertexCount = 0;

	const uint32_t* Indices = nullptr;
	uint32_t IndexCount = 0;
};

struct TangentSpaceOutput_s
{
	std::vector<float3> Positions;
	std::vector<float3> Normals;
	std::vector<float2> Texcoords[kTangentSpaceMaxTexcoords];
	std::vector<float4> Tangents;
	std::vector<uint32_t> Indices;
};

// Generates per-vertex tangents with MikkTSpace, splitting source vertices where the tangent frame
// is discontinuous. Face order and index count are preserved, so surface index ranges stay valid.
// Source vertices are only ever split, never merged, so like Unreal a vertex only welds when every
// texcoord channel agrees, which lets the extra channels ride along with the source vertex.
bool GenerateTangents(const TangentSpaceInput_s& Input, TangentSpaceOutput_s& Output);
