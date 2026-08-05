#pragma once

#include <SurfMath.h>

struct CameraFragment_s
{
	uint32_t ScreenWidth = 0;
	uint32_t ScreenHeight = 0;
	float NearZ = 0.1f;
	float FarZ = 10'000.0f;
	float Fov = 45.0f;
	float AspectRatio = 0.0f;
	matrix Projection;

	void Resize(u32 InScreenWdith, u32 InScreenHeight);
	void SetNearFar(float InNearZ, float InFarZ);
	void SetFov(float InFov);

	const matrix& GetProjection() const noexcept { return Projection; }
	Frustum CalculateViewFrustum() const noexcept;
};