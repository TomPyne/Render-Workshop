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

	/* Uses app main viewport size */
	matrix CalculateProjectionMatrix() const;

	/* Uses app main viewport size */
	matrix CalculateViewProjectionMatrix() const;

	float2 ScreenPosToNDC(int2 ScreenPosition) const;

	/* Take screenpos like cursor pos and return a start and end using the near and far positions of the NDC transformed to world space */
	void CalculateRayForScreenPosition(int2 ScreenPosition, float3& OutStart, float3& OutEnd) const;

	float NearZ = 0.1f;
	float FarZ = 10'000.0f;
	float Fov = 45.0f;

	bool Enabled = true;
};