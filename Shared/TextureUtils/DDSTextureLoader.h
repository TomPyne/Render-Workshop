#pragma once

#include <Render/Render.h>

#include <vector>

struct DDSTexture_s
{
	struct Subresource_s
	{
		uint32_t DataOffset;
		uint32_t RowPitch;
		uint32_t SlicePitch;
	};

	enum class Dimension_e : uint8_t
	{
		UNKNOWN,
		ONEDIM,
		TWODIM,
		THREEDIM,
	};

	uint32_t Width;
	uint32_t Height;
	uint32_t DepthOrArraySize;
	Dimension_e Dimension;
	rl::RenderFormat Format;
	uint32_t MipCount;
	bool Cubemap;

	// Depth/array slice -> Mips
	std::vector<Subresource_s> SubResourceInfos;

	std::vector<uint8_t> Data;
};

bool LoadDDSTexture(const wchar_t* FilePath, DDSTexture_s* Asset);