#pragma once

#include <Render/RenderTypes.h>
#include <string>

enum class TextureAssetDimension_e : uint8_t
{
	UNKNOWN,
	ONEDIM,
	TWODIM,
	THREEDIM,
};

struct TextureAssetSubResource_s
{
	uint32_t DataOffset;
	uint32_t RowPitch;
	uint32_t SlicePitch;
};

#define RTEX_VERSION 0

struct TextureAsset_s
{
	uint32_t Version = RTEX_VERSION;
	uint32_t Width;
	uint32_t Height;
	uint32_t DepthOrArraySize;
	TextureAssetDimension_e Dimension;
	rl::RenderFormat Format;
	uint32_t MipCount;
	bool Cubemap;

	// Depth/array slice -> Mips
	std::vector<TextureAssetSubResource_s> SubResourceInfos;

	std::vector<uint8_t> Data;

	std::wstring SourcePath;
};


TextureAsset_s* LoadTextureAsset(const std::wstring& FilePath);