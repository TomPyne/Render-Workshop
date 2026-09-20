#pragma once

#include <memory>

struct JsonValue_s;
struct Texture_s;
struct Path_s;

namespace TextureManager
{
	std::shared_ptr<Texture_s> RequestTexture(const Path_s& Path, bool ErrorTextureIfMissing = true);
	std::shared_ptr<Texture_s> RequestTexture(const JsonValue_s& Data, bool ErrorTextureIfMissing= true);
}