#pragma once

#include <Render/RenderTypes.h>
#include <SurfMath.h>

struct Texture_s
{
	rl::TexturePtr Texture = {};
	rl::ShaderResourceViewPtr SRV = {};
	uint2 Size = {};
	u32 MipCount = 0;
	rl::RenderFormat Format = rl::RenderFormat::UNKNOWN;

	bool Ready = false;
};