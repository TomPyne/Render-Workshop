#pragma once

#include <Render/RenderTypes.h>
#include <Render/Textures.h>
#include <SurfMath.h>

#include <atomic>

struct Texture_s
{
	rl::TexturePtr Texture = {};
	rl::ShaderResourceViewPtr SRV = {};
	uint2 Size = {};
	u32 DepthOrArraySize = 1;
	u32 MipCount = 0;
	rl::TextureDimension Dimension = rl::TextureDimension::TEX2D;
	rl::RenderFormat Format = rl::RenderFormat::UNKNOWN;

	// Set once the handles above are valid. The GPU copy may still be in flight, use IsReady before sampling
	std::atomic<bool> Ready;

	bool IsReady() const;
};