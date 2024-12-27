#pragma once

#include <Render/RenderTypes.h>
#include <string>

struct TextureAsset_s
{
	tpr::TexturePtr Texture;
	tpr::ShaderResourceViewPtr SRV;

#if _DEBUG
	std::wstring DebugName;
#endif
};

TextureAsset_s LoadTexture(const std::wstring& FilePath, bool GenerateMips = false);
TextureAsset_s LoadTexture(const std::string& FilePath, bool GenerateMips = false);