#pragma once

#include "SpatialObjectComponent.h"

class CameraComponent_c : public SpatialObjectComponent_c
{
public:
	using SpatialObjectComponent_c::SpatialObjectComponent_c;
	virtual ~CameraComponent_c() = default;

	virtual void OnCreate() override;

	virtual void PreDestroy() override;

	matrix CalculateViewMatrix() const;

	matrix CalculateProjectionMatrix(float AspectRatio) const;
	matrix CalculateProjectionMatrix(u32 ScreenWidth, u32 ScreenHeight) const;

	float NearZ = 0.1f;
	float FarZ = 10'000.0f;
	float Fov = 45.0f;
};