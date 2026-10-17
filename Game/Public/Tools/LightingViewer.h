#pragma once

class CameraComponent_c;
class SpaceRenderer_c;
struct RenderScene_s;

namespace LightingViewer
{
	// Exposure, sun, shadow denoiser and skylight settings. The shadow stages are easiest to judge in the Lighting view mode
	void DrawWindow(bool* Open, SpaceRenderer_c* Renderer, CameraComponent_c* Camera, RenderScene_s* Scene);
}
