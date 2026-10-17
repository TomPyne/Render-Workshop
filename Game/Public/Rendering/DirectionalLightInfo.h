#pragma once

#include <SurfMath.h>

// Scene side description of a directional light, owned by DirectionalLightComponent and registered with the RenderScene
struct DirectionalLightInfo_s
{
	// Unit vector pointing towards the light, derived from the component's world rotation
	float3 Direction = float3(0.0f, 1.0f, 0.0f);
	float IlluminanceLux = 20000.0f;
	// Chroma only, normalised to luminance 1 before use so brightness comes from IlluminanceLux
	float3 Color = float3(1.0f, 0.95f, 0.85f);
	// Radians, angular radius of the light used for soft shadows
	float SoftAngle = 0.02f;
};
