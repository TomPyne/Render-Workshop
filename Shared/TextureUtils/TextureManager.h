#pragma once

#include <Render/RenderTypes.h>
#include <string>

struct TextureAsset_s
{
	uint32_t Width;
	uint32_t Height;
	tpr::RenderFormat Format;
	uint32_t MipCount;

	std::vector<std::vector<uint8_t>> MipPixels;

	std::wstring SourcePath;
};


TextureAsset_s* LoadTextureAsset(const std::wstring& FilePath);