#pragma once

#include <cstdint>

struct CameraInfo_s;

namespace GameStats
{
	void UpdateCamera(const CameraInfo_s* Camera);
	void UpdatePrimCount(uint32_t PrimCount);
	
	void DrawGameStatsWindow(bool* Open);
}