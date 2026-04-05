#pragma once

#include <SurfMath.h>

struct Camera
{
	void Resize(u32 InScreenWdith, u32 InScreenHeight);
	void SetNearFar(float InNearZ, float InFarZ);
	void SetFov(float InFov);

	const matrix& GetProjection() const noexcept { return Projection; }
	const matrix& GetPixelProjection() const noexcept { return PixelProjection; }
	Frustum CalculateViewFrustum() const noexcept;

protected:
	u32 ScreenWidth = 0;
	u32 ScreenHeight = 0;
	float NearZ = 0.1f;
	float FarZ = 10'000.0f;
	float Fov = 45.0f;
	float AspectRatio = 0.0f;
	matrix Projection;
	matrix PixelProjection;

	void RecalculateMatrices();
};