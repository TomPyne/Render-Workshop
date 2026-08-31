#pragma once

#include "Object/SpatialObjectComponent.h"

class CameraComponent_c : public SpatialObjectComponent_c
{
	OBJECTCOMPONENT_BODY(CameraComponent_c, SpatialObjectComponent_c)

	// Begin SpatialObjectComponent_c
	virtual void OnCreate() override;
	virtual void PreDestroy() override;
	// End SpatialObjectComponent_c

	matrix CalculateViewMatrix() const;

	matrix CalculateProjectionMatrix(float AspectRatio) const;
	matrix CalculateProjectionMatrix(u32 ScreenWidth, u32 ScreenHeight) const;

	float NearZ = 0.1f;
	float FarZ = 10'000.0f;
	float Fov = 45.0f;

	bool Enabled = true;
};