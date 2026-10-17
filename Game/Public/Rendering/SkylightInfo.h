#pragma once

#include "Rendering/LightingUnits.h"

#include <cstdint>
#include <memory>

// Scene side description of a skylight, owned by SkylightComponent and registered with the RenderScene
struct SkylightInfo_s
{
	// Equirectangular HDR
	std::shared_ptr<struct Texture_s> EnvironmentMap;
	// Nits per unit of EnvironmentMap, which has no physical scale of its own.
	// The default matches the look from before physical units
	float LuminanceScale = 1.0f / kPreExposure;
	// Radians, rotates the environment about world up. Baked into the cubes
	float Yaw = 0.0f;
	// Unique across all skylights, changes whenever EnvironmentMap does
	uint32_t Generation = 0;
};
