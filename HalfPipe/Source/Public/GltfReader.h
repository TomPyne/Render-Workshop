#pragma once

#include <SurfMath.h>

#include <cstdint>
#include <string>
#include <vector>

// Minimal binary glTF (.glb) reader. Loads the triangle geometry of every mesh node in the default
// scene with node transforms baked in, and splits it by material the way WaveFrontReader_c does.
// Material parameters, skins, morph targets, compression and required extensions are rejected or
// ignored.
class GltfReader_c
{
public:

	static constexpr uint32_t MaxTexcoords = 4;

	struct Vertex_s
	{
		float3 Position;
		float3 Normal;
		float2 Texcoords[MaxTexcoords];
	};

	std::vector<Vertex_s>		Vertices;
	std::vector<uint32_t>		Indices;
	// Material slot of each triangle, indexing MaterialNames.
	std::vector<uint32_t>		Attributes;
	// Slot 0 is a synthetic "default" for primitives without a material, glTF material N is slot N + 1.
	std::vector<std::wstring>	MaterialNames;
	// Highest channel count across all primitives, channels a primitive lacks are zero filled.
	uint32_t					TexcoordCount = 0;

	std::wstring				Name;

	bool Load(const wchar_t* FileName);

	void Clear();
};
