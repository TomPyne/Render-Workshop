#pragma once

class DistanceFieldScene_c;

namespace DistanceFieldViewer
{
	// Draws one slice of a loaded mesh's distance field from its CPU copy, blue inside and orange outside,
	// and the instances in Scene when it is not null
	void DrawWindow(bool* Open, const DistanceFieldScene_c* Scene);
}
