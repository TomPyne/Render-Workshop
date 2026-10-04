#pragma once

class SpaceRenderer_c;

namespace DistanceFieldViewer
{
	// Draws one slice of a loaded mesh's distance field from its CPU copy, blue inside and orange outside,
	// and the renderer's distance field instances and global volume settings when it is not null
	void DrawWindow(bool* Open, SpaceRenderer_c* Renderer);
}
