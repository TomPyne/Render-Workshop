#pragma once

#include <cstdint>

class CameraComponent_c;

namespace GameStats
{
	void UpdateCamera(CameraComponent_c* Camera);
	void UpdatePrimCount(uint32_t PrimCount);
	
	void DrawGameStatsWindow(bool* Open);
}