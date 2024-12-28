#pragma once

#include <cstdint>
#include <vector>

namespace MeshProcessing
{
	using index_t = uint32_t;

	bool CleanMesh(uint32_t* Indices, size_t NumFaces, size_t NumVerts, const uint32_t* Attributes, std::vector<uint32_t>& DupedVerts, bool BreakBowTies = false);
	bool AttributeSort(size_t NumFaces, uint32_t* Attributes, uint32_t* FaceRemap);
	bool ReorderIndices(uint32_t* Indices, size_t NumFaces, const uint32_t* FaceRemap, uint32_t* OutIndices);
	bool OptimizeFacesLRU(uint32_t* Indices, size_t NumFaces, uint32_t* FaceRemap);
}