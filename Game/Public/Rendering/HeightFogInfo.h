#pragma once

#include <SurfMath.h>

struct HeightFogInfo_s
{
	bool Enabled = true;

	// All units in metres
	float Density = 0.02f;
	float HeightFalloff = 0.2f;
	float BaseHeight = 0.0f;
	float StartDistance = 0.0f; 
	float MaxOpacity = 1.0f;
	float SkyDistance = 1000.0f;
	float3 Albedo = float3(1.0f, 1.0f, 1.0f);
	float SunAnisotropy = 0.6f;
	float SunScatterScale = 1.0f;
};
