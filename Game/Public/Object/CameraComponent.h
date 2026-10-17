#pragma once

#include "Object/SpatialObjectComponent.h"
#include "Rendering/CameraInfo.h"

class CameraComponent_c : public SpatialObjectComponent_c
{
	OBJECTCOMPONENT_BODY(CameraComponent_c, SpatialObjectComponent_c)

	// Begin SpatialObjectComponent_c
	virtual void OnCreate() override;
	virtual void Deserialize(const struct JsonValue_s& Data) override;
	virtual void Update(float Delta) override;
	virtual void PreDestroy() override;
	virtual void OnTransformed() override;
	// End SpatialObjectComponent_c

	// Registered with the RenderScene by the Space while this is the active camera
	const CameraInfo_s* GetCameraInfo() const { return &CameraInfo; }

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
	// Exposure value at ISO 100, a sunny day is about 15
	float ExposureEV100 = 15.0f;

	bool Enabled = true;

protected:

	void UpdateCameraInfoTransform();
	void UpdateCameraInfoSettings();

	CameraInfo_s CameraInfo;
};