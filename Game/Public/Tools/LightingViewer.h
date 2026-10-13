#pragma once

class SpaceRenderer_c;

namespace LightingViewer
{
	// Shadow denoiser and skylight settings. The shadow stages are easiest to judge in the Lighting view mode
	void DrawWindow(bool* Open, SpaceRenderer_c* Renderer);
}
