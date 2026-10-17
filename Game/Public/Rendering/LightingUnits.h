#pragma once

#include <SurfMath.h>

#include <cmath>

// Lights are authored in physical units, illuminance in lux and luminance in nits (cd/m^2).
// The lit scene buffer holds nits * kPreExposure so values sit in a comfortable FP16 range.
// Emissive, particle colours and the bloom threshold are still authored directly in that scaled space.
// Fixed, chosen so 100,000 lux of sun matches the look from before physical units
constexpr float kPreExposure = 2.86e-5f;

// Rec.709 luminance, so a light's colour carries chroma only and its intensity sets brightness
inline float3 NormalizeToUnitLuminance(float3 Color)
{
	const float Luminance = 0.2126f * Color.x + 0.7152f * Color.y + 0.0722f * Color.z;
	return Luminance > 0.0f ? Color / Luminance : float3(0.0f, 0.0f, 0.0f);
}

// Multiplier from luminance in nits to tonemapper input. Saturation based sensitivity, 78 / (0.65 * 100) = 1.2
// (Lagarde and de Rousiers, Moving Frostbite to PBR)
inline float ExposureFromEV100(float EV100)
{
	return 1.0f / (1.2f * std::exp2(EV100));
}
