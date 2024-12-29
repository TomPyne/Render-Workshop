#pragma once

#include <cstdint>
#include <SurfMath.h>
#include <vector>

namespace MeshProcessing
{
	using index_t = uint32_t;

	struct Subset_s
	{
		uint32_t Offset;
		uint32_t Count;
	};

	bool CleanMesh(index_t* Indices, size_t NumFaces, size_t NumVerts, const uint32_t* Attributes, std::vector<uint32_t>& DupedVerts, bool BreakBowTies = false);
	bool AttributeSort(size_t NumFaces, uint32_t* Attributes, uint32_t* FaceRemap);
	bool ReorderIndices(index_t* Indices, size_t NumFaces, const uint32_t* FaceRemap, index_t* OutIndices) noexcept;
	bool OptimizeFacesLRU(index_t* Indices, size_t NumFaces, uint32_t* FaceRemap);
	bool OptimizeVertices(index_t* Indices, size_t NumFaces, size_t NumVerts, uint32_t* VertexRemap) noexcept;
	bool FinalizeIndices(index_t* Indices, size_t NumFaces, const uint32_t* VertexRemap, size_t NumVerts, index_t* OutIndices) noexcept;
	bool FinalizeVertices(void* Vertices, size_t Stride, size_t NumVerts, const uint32_t* VertexRemap) noexcept;
	bool FinalizeVertices(void* Vertices, size_t Stride, size_t NumVerts, const uint32_t* DupedVerts, size_t NumDupedVerts, const uint32_t* VertexRemap, void* OutVertices) noexcept;

	std::vector<Subset_s> ComputeSubsets(const uint32_t* Attributes, size_t NumFaces);

	bool ComputeNormals(const index_t* Indices, size_t NumFaces, const float3* Positions, size_t NumVerts, float3* Normals) noexcept;
	bool ComputeTangents(const index_t* Indices, size_t NumFaces, const float3* Positions, const float3* Normals, const float2* Texcoords, size_t NumVerts, float4* Tangents, float3* Bitangents) noexcept;


	struct Meshlet_s
	{
		uint32_t VertCount;
		uint32_t VertOffset;
		uint32_t PrimCount;
		uint32_t PrimOffset;
	};

	union PackedTriangle_s
	{
		struct
		{
			uint32_t I0 : 10;
			uint32_t I1 : 10;
			uint32_t I2 : 10;
			uint32_t __pad : 2;
		} Indices;
		uint32_t packed;
	};

	bool ComputeMeshlets(
		uint32_t maxVerts, uint32_t maxPrims,
		const index_t* Indices, uint32_t indexCount,
		const Subset_s* indexSubsets, uint32_t subsetCount,
		const float3* positions, uint32_t vertexCount,
		std::vector<Subset_s>& meshletSubsets,
		std::vector<Meshlet_s>& meshlets,
		std::vector<uint8_t>& uniqueVertexIndices,
		std::vector<PackedTriangle_s>& primitiveIndices);
}