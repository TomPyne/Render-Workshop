#pragma once

#include <cstdint>
#include <vector>

namespace MeshProcessing
{
	using index_t = uint32_t;

	bool CleanMesh(index_t* Indices, size_t NumFaces, size_t NumVerts, const uint32_t* Attributes, std::vector<uint32_t>& DupedVerts, bool BreakBowTies = false);
	bool AttributeSort(size_t NumFaces, uint32_t* Attributes, uint32_t* FaceRemap);
	bool ReorderIndices(index_t* Indices, size_t NumFaces, const uint32_t* FaceRemap, index_t* OutIndices);
	bool OptimizeFacesLRU(index_t* Indices, size_t NumFaces, uint32_t* FaceRemap);
	bool OptimizeVertices(index_t* Indices, size_t NumFaces, size_t NumVerts, uint32_t* VertexRemap);
	bool FinalizeIndices(index_t* Indices, size_t NumFaces, const uint32_t* VertexRemap, size_t NumVerts, index_t* OutIndices);
}