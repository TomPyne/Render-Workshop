#pragma once

#include <string>

struct MeshAsset_s
{
	struct Surface_s
	{
		// Material
		// Buffers
	};
	// Surfaces
	// Bounding Box
};

namespace MeshAsset
{
	MeshAsset_s* RequestMeshAsset(const std::wstring& Path);
}