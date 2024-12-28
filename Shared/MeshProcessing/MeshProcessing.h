#pragma once

#include <cstdint>
#include <SurfMath.h>
#include <vector>

namespace MeshProcessing
{
	using index_t = uint32_t;

	bool CleanMesh(index_t* Indices, size_t NumFaces, size_t NumVerts, const uint32_t* Attributes, std::vector<uint32_t>& DupedVerts, bool BreakBowTies = false);
	bool AttributeSort(size_t NumFaces, uint32_t* Attributes, uint32_t* FaceRemap);
	bool ReorderIndices(index_t* Indices, size_t NumFaces, const uint32_t* FaceRemap, index_t* OutIndices) noexcept;
	bool OptimizeFacesLRU(index_t* Indices, size_t NumFaces, uint32_t* FaceRemap);
	bool OptimizeVertices(index_t* Indices, size_t NumFaces, size_t NumVerts, uint32_t* VertexRemap) noexcept;
	bool FinalizeIndices(index_t* Indices, size_t NumFaces, const uint32_t* VertexRemap, size_t NumVerts, index_t* OutIndices) noexcept;
	bool FinalizeVertices(void* Vertices, size_t Stride, size_t NumVerts, const uint32_t* VertexRemap) noexcept;
	std::vector<std::pair<size_t, size_t>> ComputeSubsets(const uint32_t* Attributes, size_t NumFaces);
	bool ComputeNormals(const index_t* Indices, size_t NumFaces, const float3* Positions, size_t NumVerts, float3* Normals) noexcept;
	bool ComputeTangents(const index_t* Indices, size_t NumFaces, const float3* Positions, const float3* Normals, const float2* Texcoords, size_t NumVerts, float4* Tangents, float3* Bitangents) noexcept;
}