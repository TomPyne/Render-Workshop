#pragma once

#include "Camera.h"

struct FlyCamera : public Camera
{
	void SetView(const float3& InPosition, float Pitch, float Yaw);
	void UpdateView(float Delta);

	const matrix& GetView() const noexcept { return View; }
	const float3& GetPosition() const noexcept { return Position; }

	void SetPosition(const float3& InPosition) noexcept { Position = InPosition; }

	Frustum GetWorldFrustum() const;

protected:
	float3 Position;
	float3 LookDir;
	float CamPitch = 0.0f;
	float CamYaw = 0.0f;
	matrix View;
};