#pragma once

#include <cstdint>
#include <SurfMath.h>
#include <vector>

namespace MeshProcessing
{
	using index_t = uint32_t;

	using Subset_t = std::pair<size_t, size_t>;

	bool CleanMesh(index_t* Indices, size_t NumFaces, size_t NumVerts, const uint32_t* Attributes, std::vector<uint32_t>& DupedVerts, bool BreakBowTies = false);
	bool AttributeSort(size_t NumFaces, uint32_t* Attributes, uint32_t* FaceRemap);
	bool ReorderIndices(index_t* Indices, size_t NumFaces, const uint32_t* FaceRemap, index_t* OutIndices) noexcept;
	bool OptimizeFacesLRU(index_t* Indices, size_t NumFaces, uint32_t* FaceRemap);
	bool OptimizeVertices(index_t* Indices, size_t NumFaces, size_t NumVerts, uint32_t* VertexRemap) noexcept;
	bool FinalizeIndices(index_t* Indices, size_t NumFaces, const uint32_t* VertexRemap, size_t NumVerts, index_t* OutIndices) noexcept;
	bool FinalizeVertices(void* Vertices, size_t Stride, size_t NumVerts, const uint32_t* VertexRemap) noexcept;
	bool FinalizeVertices(void* Vertices, size_t Stride, size_t NumVerts, const uint32_t* DupedVerts, size_t NumDupedVerts, const uint32_t* VertexRemap, void* OutVertices) noexcept;

	std::vector<Subset_t> ComputeSubsets(const uint32_t* Attributes, size_t NumFaces);

	bool ComputeNormals(const index_t* Indices, size_t NumFaces, const float3* Positions, size_t NumVerts, float3* Normals) noexcept;
	bool ComputeTangents(const index_t* Indices, size_t NumFaces, const float3* Positions, const float3* Normals, const float2* Texcoords, size_t NumVerts, float4* Tangents, float3* Bitangents) noexcept;

	struct Meshlet_s
	{
		uint32_t VertCount;
		uint32_t VertOffset;
		uint32_t PrimCount;
		uint32_t PrimOffset;
	};

	struct MeshletTriangle_s
	{
		uint32_t I0 : 10;
		uint32_t I1 : 10;
		uint32_t I2 : 10;
	};

	bool ComputeMeshlets(
		const index_t* Indices, size_t NumFaces, 
		const float3* Positions, size_t NumVerts, 
		const Subset_t* Subsets, size_t NumSubsets, 
		std::vector<Meshlet_s>& Meshlets, std::vector<uint8_t>& UniqueVertexIndices, std::vector<MeshletTriangle_s>& PrimitiveIndices, 
		Subset_t* OutMeshletSubsets, 
		size_t MaxVerts = 128, size_t MaxPrims = 128);
}