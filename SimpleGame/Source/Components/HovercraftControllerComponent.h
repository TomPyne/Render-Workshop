#pragma once

#include <Object/ControllerComponent.h>

#include <SurfMath.h>

class HovercraftObject_c;

namespace HovercraftMovementMode_e
{
	enum Type
	{
		HULL_RELATIVE,
		TURRET_RELATIVE,
		COUNT
	};
}

// Flight, camera and aiming for HovercraftObject_c. Angles are in radians, tuning values in degrees where named.
class HovercraftControllerComponent_c : public ControllerComponent_c
{
	OBJECTCOMPONENT_BODY(HovercraftControllerComponent_c, ControllerComponent_c)

	// Begin ObjectComponent_c
	virtual void Update(float Delta) override;
	// End ObjectComponent_c

	// Places the craft at min hover height over Position and clears all motion. The movement mode is kept.
	void ResetTo(float3 Position, float Yaw);

	float3 GetTargetPoint() const { return TargetPoint; }
	float3 GetTurretAimDirection() const { return TurretAimDirection; }

	// Tuning
	float MinHoverHeight = 3.0f;
	float MaxHoverHeight = 20.0f;
	float HoverHeightRate = 8.0f;
	float HoverSpringFrequency = 5.0f;
	float HoverSpringDamping = 0.6f;

	float TopSpeed = 20.0f;
	float TimeToTopSpeed = 0.6f;
	float StopTime = 0.4f;
	float StopSpeed = 0.05f;

	float HullTurnRateDegrees = 90.0f;

	float CameraOffsetBack = 20.0f;
	float CameraOffsetUp = 4.0f;
	// Positive pitch looks down
	float CameraMaxPitchDownDegrees = 80.0f;
	float CameraMaxPitchUpDegrees = 10.0f;
	float CameraDefaultPitchDegrees = 10.0f;
	float MouseSensitivityDegrees = 0.15f;

	float TargetMaxRange = 300.0f;
	float CursorMaxDistance = 100.0f;
	float CursorScale = 2.0f;

	float MaxLeanDegrees = 12.0f;
	float LeanSmoothing = 6.0f;

protected:
	HovercraftObject_c* GetHovercraft() const;

	// Returns false while another controller is on top. Reapplies the mouse lock when this one comes back on top.
	bool UpdateActive();

	void UpdateCameraAngles();
	void UpdateMovement(float Delta);
	void UpdateTarget(HovercraftObject_c& Hovercraft);
	void UpdateHullYaw(float Delta);
	void ApplyTransforms(HovercraftObject_c& Hovercraft);
	void UpdateLean(HovercraftObject_c& Hovercraft, float Delta);

	float3 Velocity = float3(0.0f);
	float3 PrevVelocity = float3(0.0f);
	float VerticalVelocity = 0.0f;
	float TargetHoverHeight = 3.0f;
	float3 Position = float3(0.0f);

	float HullYaw = 0.0f;
	float CameraYaw = 0.0f;
	float CameraPitch = 0.0f;

	float3 TargetPoint = float3(0.0f);
	float3 TurretAimDirection = float3(0.0f, 0.0f, 1.0f);

	float LeanPitch = 0.0f;
	float LeanRoll = 0.0f;

	HovercraftMovementMode_e::Type MovementMode = HovercraftMovementMode_e::HULL_RELATIVE;

	bool WantsMouseLock = true;
	bool WasActive = false;
};
