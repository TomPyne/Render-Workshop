#pragma once

#include <Render/RenderTypes.h>
#include <SurfMath.h>

#include <atomic>

struct Texture_s
{
	rl::TexturePtr Texture = {};
	rl::ShaderResourceViewPtr SRV = {};
	uint2 Size = {};
	u32 MipCount = 0;
	rl::RenderFormat Format = rl::RenderFormat::UNKNOWN;

	// Set once the handles above are valid. The GPU copy may still be in flight, use IsReady before sampling
	std::atomic<bool> Ready;

	bool IsReady() const;
};