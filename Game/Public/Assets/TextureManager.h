#pragma once

#include <memory>

struct JsonValue_s;
struct Texture_s;
struct Path_s;

namespace TextureManager
{
	// Returns immediately with the texture not ready, the load runs as a job. Blocking waits for the CPU side of the load, the GPU upload may still be in flight
	std::shared_ptr<Texture_s> RequestTexture(const Path_s& Path, bool ErrorTextureIfMissing = true, bool Blocking = false);
	std::shared_ptr<Texture_s> RequestTexture(const JsonValue_s& Data, bool ErrorTextureIfMissing = true, bool Blocking = false);
}