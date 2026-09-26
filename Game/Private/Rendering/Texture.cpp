#include "Rendering/Texture.h"

#include <Render/Textures.h>

bool Texture_s::IsReady() const
{
	return Ready.load(std::memory_order_acquire) && rl::IsTextureUploadComplete(Texture);
}
