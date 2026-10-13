#pragma once

#include <cstdint>
#include <memory>

// Scene side description of a skylight, owned by SkylightComponent and registered with the RenderScene
struct SkylightInfo_s
{
	// Equirectangular HDR
	std::shared_ptr<struct Texture_s> EnvironmentMap;
	float Intensity = 1.0f;
	// Radians, rotates the environment about world up. Baked into the cubes
	float Yaw = 0.0f;
	// Unique across all skylights, changes whenever EnvironmentMap does
	uint32_t Generation = 0;
};
