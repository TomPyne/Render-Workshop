#pragma once

#include <SurfMath.h>

// Render side copy of the primary camera, owned by CameraComponent and registered with the RenderScene by the Space
struct CameraInfo_s
{
	float3 Position = float3(0.0f);
	// Unit vector
	float3 Forward = float3(0.0f, 0.0f, 1.0f);
	// Degrees, vertical
	float Fov = 45.0f;
	float NearZ = 0.1f;
	float FarZ = 10'000.0f;
	// Exposure value at ISO 100, a sunny day is about 15
	float ExposureEV100 = 15.0f;

	matrix CalculateViewMatrix() const
	{
		return MakeMatrixLookAtLH(Position, Position + Forward, float3{ 0.0f, 1.0f, 0.0f });
	}

	matrix CalculateProjectionMatrix(u32 ScreenWidth, u32 ScreenHeight) const
	{
		const float AspectRatio = static_cast<float>(ScreenWidth) / static_cast<float>(ScreenHeight);
		return MakeMatrixPerspectiveFovLH(ConvertToRadians(Fov), AspectRatio, NearZ, FarZ);
	}
};
