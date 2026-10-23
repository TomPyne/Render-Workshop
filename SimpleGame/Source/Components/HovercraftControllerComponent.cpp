#include "HovercraftControllerComponent.h"

#include "Objects/HovercraftObject.h"
#include "Objects/SelectionCursorObject.h"
#include "World/Ground.h"

#include <Input/Input.h>
#include <Object/CameraComponent.h>
#include <Object/MeshComponent.h>
#include <Shared/Logging/Logging.h>

namespace
{
	const float3 Up = float3{ 0.0f, 1.0f, 0.0f };
	const float3 Right = float3{ 1.0f, 0.0f, 0.0f };
	const float3 Forward = float3{ 0.0f, 0.0f, 1.0f };

	float WrapAngle(float Angle)
	{
		Angle = fmodf(Angle + K_PI, 2.0f * K_PI);
		return (Angle < 0.0f ? Angle + 2.0f * K_PI : Angle) - K_PI;
	}

	quat YawRotation(float Yaw)
	{
		return QuatFromAxisAngle(Up, Yaw);
	}

	// Pitch about our own right axis, then yaw about world up.
	quat PitchYawRotation(float Pitch, float Yaw)
	{
		return Mul(QuatFromAxisAngle(Right, Pitch), YawRotation(Yaw));
	}

	float3 Horizontal(float3 V)
	{
		return float3{ V.x, 0.0f, V.z };
	}

	// Linear drag gets within 5% of its end state in 3 / drag seconds.
	float DragForSettleTime(float SettleTime)
	{
		return 3.0f / Max(SettleTime, 0.01f);
	}
}

HovercraftObject_c* HovercraftControllerComponent_c::GetHovercraft() const
{
	return dynamic_cast<HovercraftObject_c*>(GetOwner());
}

void HovercraftControllerComponent_c::ResetTo(float3 InPosition, float Yaw)
{
	HovercraftObject_c* Hovercraft = GetHovercraft();
	if (!Hovercraft)
		return;

	Position = float3{ InPosition.x, GetGroundHeight(InPosition.x, InPosition.z) + MinHoverHeight, InPosition.z };
	Velocity = float3(0.0f);
	PrevVelocity = float3(0.0f);
	VerticalVelocity = 0.0f;
	TargetHoverHeight = MinHoverHeight;

	HullYaw = WrapAngle(Yaw);
	CameraYaw = HullYaw;
	CameraPitch = ConvertToRadians(CameraDefaultPitchDegrees);

	LeanPitch = 0.0f;
	LeanRoll = 0.0f;

	UpdateTarget(*Hovercraft);
	ApplyTransforms(*Hovercraft);

	if (MeshComponent_c* Mesh = Hovercraft->GetMeshComponent())
	{
		Mesh->SetRotation(quat::Identity());
	}
}

void HovercraftControllerComponent_c::Update(float Delta)
{
	Super::Update(Delta);

	HovercraftObject_c* Hovercraft = GetHovercraft();
	if (!Hovercraft)
		return;

	if (!UpdateActive())
		return;

	if (Input::IsKeyPressed(KeyCode_e::_F5))
	{
		switch (MovementMode)
		{
		case HovercraftMovementMode_e::HULL_RELATIVE:
			MovementMode = HovercraftMovementMode_e::TURRET_RELATIVE;
			LOGINFO("[Hovercraft] Movement mode: turret-relative");
			break;
		default:
			MovementMode = HovercraftMovementMode_e::HULL_RELATIVE;
			LOGINFO("[Hovercraft] Movement mode: hull-relative");
			break;
		}
	}

	if (Hovercraft->IsDead() || Delta <= 0.0f)
		return;

	if (Input::IsKeyPressed(KeyCode_e::_K))
	{
		Hovercraft->Die();
		return;
	}

	Position = Hovercraft->GetTransform().GetPosition();

	// Movement runs first and the camera is placed last, so the view and target use this frame's final hull position and yaw.
	UpdateCameraAngles();
	UpdateMovement(Delta);
	UpdateTarget(*Hovercraft);
	UpdateHullYaw(Delta);
	ApplyTransforms(*Hovercraft);
	UpdateLean(*Hovercraft, Delta);
}

bool HovercraftControllerComponent_c::UpdateActive()
{
	if (!IsActiveController())
	{
		WasActive = false;
		return false;
	}

	if (!WasActive)
	{
		Input::SetMouseCaptured(WantsMouseLock);
		WasActive = true;
	}
	else if (WantsMouseLock && !Input::IsMouseCaptured())
	{
		// Released by something else, such as the window losing focus, so wait for the toggle to grab it again
		WantsMouseLock = false;
	}

	if (Input::IsKeyPressed(KeyCode_e::_ESC))
	{
		WantsMouseLock = !WantsMouseLock;
		Input::SetMouseCaptured(WantsMouseLock);
	}

	return true;
}

void HovercraftControllerComponent_c::UpdateCameraAngles()
{
	if (Input::IsMouseCaptured())
	{
		const int2 MouseDelta = Input::GetMouseDelta();
		const float Sensitivity = ConvertToRadians(MouseSensitivityDegrees);
		CameraYaw = WrapAngle(CameraYaw + Sensitivity * static_cast<float>(MouseDelta.x));
		CameraPitch += Sensitivity * static_cast<float>(MouseDelta.y);
	}

	CameraPitch = Clamp(CameraPitch, -ConvertToRadians(CameraMaxPitchUpDegrees), ConvertToRadians(CameraMaxPitchDownDegrees));
}

void HovercraftControllerComponent_c::UpdateMovement(float Delta)
{
	// Horizontal
	float3 MoveInput = float3(0.0f);
	if (Input::IsKeyDown(KeyCode_e::_W)) MoveInput += Forward;
	if (Input::IsKeyDown(KeyCode_e::_S)) MoveInput -= Forward;
	if (Input::IsKeyDown(KeyCode_e::_D)) MoveInput += Right;
	if (Input::IsKeyDown(KeyCode_e::_A)) MoveInput -= Right;

	const bool HasInput = Length(MoveInput) > 0.0f;

	float MoveYaw = HullYaw;
	const float3 TurretAimFlat = Horizontal(TurretAimDirection);
	if (MovementMode == HovercraftMovementMode_e::TURRET_RELATIVE && Length(TurretAimFlat) > 1e-3f)
	{
		MoveYaw = atan2f(TurretAimFlat.x, TurretAimFlat.z);
	}

	const float3 MoveDir = HasInput ? Normalize(Rotate(YawRotation(MoveYaw), MoveInput)) : float3(0.0f);

	const float CoastDrag = DragForSettleTime(TimeToTopSpeed);
	const float BrakeDrag = DragForSettleTime(StopTime);
	const float Thrust = TopSpeed * CoastDrag;

	const bool Braking = !HasInput || Dot(MoveDir, Velocity) < 0.0f;
	const float Drag = Braking ? BrakeDrag : CoastDrag;

	PrevVelocity = Velocity;
	Velocity = Velocity * expf(-Drag * Delta) + MoveDir * (Thrust * Delta);

	if (!HasInput && Length(Velocity) < StopSpeed)
	{
		Velocity = float3(0.0f);
	}

	Position += Velocity * Delta;

	// Hover
	float HoverInput = 0.0f;
	if (Input::IsKeyDown(KeyCode_e::_SPACEBAR)) HoverInput += 1.0f;
	if (Input::IsKeyDown(KeyCode_e::_CTRL)) HoverInput -= 1.0f;

	TargetHoverHeight = Clamp(TargetHoverHeight + HoverInput * HoverHeightRate * Delta, MinHoverHeight, MaxHoverHeight);

	const float TargetY = GetGroundHeight(Position.x, Position.z) + TargetHoverHeight;
	const float SpringAccel = HoverSpringFrequency * HoverSpringFrequency * (TargetY - Position.y)
		- 2.0f * HoverSpringDamping * HoverSpringFrequency * VerticalVelocity;

	VerticalVelocity += SpringAccel * Delta;
	Position.y += VerticalVelocity * Delta;
}

void HovercraftControllerComponent_c::UpdateTarget(HovercraftObject_c& Hovercraft)
{
	const quat CameraRotation = PitchYawRotation(CameraPitch, CameraYaw);
	const float3 RayStart = Position + Rotate(YawRotation(CameraYaw), float3{ 0.0f, CameraOffsetUp, -CameraOffsetBack });
	const float3 RayDir = Rotate(CameraRotation, Forward);

	// The ground is flat, so its height under the camera is its height everywhere.
	const float GroundY = GetGroundHeight(RayStart.x, RayStart.z);

	float HitDistance = -1.0f;
	if (RayDir.y < -1e-4f)
	{
		HitDistance = (GroundY - RayStart.y) / RayDir.y;
	}

	const bool Hit = HitDistance >= 0.0f && HitDistance <= TargetMaxRange;
	TargetPoint = RayStart + RayDir * (Hit ? HitDistance : TargetMaxRange);

	// The turret sits at the hull centre until the mesh has a real one
	const float3 TurretToTarget = TargetPoint - Position;
	if (Length(TurretToTarget) > 1e-3f)
	{
		TurretAimDirection = Normalize(TurretToTarget);
	}

	if (SelectionCursorObject_c* Cursor = Hovercraft.GetTargetCursor())
	{
		if (Hit && HitDistance <= CursorMaxDistance)
		{
			Cursor->SetHit(TargetPoint, CursorScale);
		}
		else
		{
			Cursor->UnsetHit();
		}
	}
}

void HovercraftControllerComponent_c::UpdateHullYaw(float Delta)
{
	const float3 ToTarget = Horizontal(TargetPoint - Position);

	// Right under the hull the direction is noise, so hold the current facing
	if (Length(ToTarget) < 1.0f)
		return;

	const float DesiredYaw = atan2f(ToTarget.x, ToTarget.z);
	const float MaxStep = ConvertToRadians(HullTurnRateDegrees) * Delta;
	HullYaw = WrapAngle(HullYaw + Clamp(WrapAngle(DesiredYaw - HullYaw), -MaxStep, MaxStep));
}

void HovercraftControllerComponent_c::ApplyTransforms(HovercraftObject_c& Hovercraft)
{
	const quat HullRotation = YawRotation(HullYaw);

	Hovercraft.SetPosition(Position);
	Hovercraft.SetRotation(HullRotation);

	// The camera is a child of the hull, so undo the hull yaw to keep the view in world space.
	if (CameraComponent_c* Camera = Hovercraft.GetCameraComponent())
	{
		const quat InverseHull = Conjugate(HullRotation);
		const float3 WorldOffset = Rotate(YawRotation(CameraYaw), float3{ 0.0f, CameraOffsetUp, -CameraOffsetBack });

		Camera->SetPosition(Rotate(InverseHull, WorldOffset));
		Camera->SetRotation(Mul(PitchYawRotation(CameraPitch, CameraYaw), InverseHull));
	}
}

void HovercraftControllerComponent_c::UpdateLean(HovercraftObject_c& Hovercraft, float Delta)
{
	MeshComponent_c* Mesh = Hovercraft.GetMeshComponent();
	if (!Mesh)
		return;

	const float3 Acceleration = (Velocity - PrevVelocity) / Delta;
	const float3 LocalAcceleration = Rotate(Conjugate(YawRotation(HullYaw)), Acceleration);

	// Full lean at full thrust
	const float MaxLean = ConvertToRadians(MaxLeanDegrees);
	const float LeanPerAccel = MaxLean / (TopSpeed * DragForSettleTime(TimeToTopSpeed));

	// Positive pitch dips the nose, negative roll drops the right side.
	const float TargetPitch = Clamp(LocalAcceleration.z * LeanPerAccel, -MaxLean, MaxLean);
	const float TargetRoll = Clamp(-LocalAcceleration.x * LeanPerAccel, -MaxLean, MaxLean);

	const float Blend = 1.0f - expf(-LeanSmoothing * Delta);
	LeanPitch += (TargetPitch - LeanPitch) * Blend;
	LeanRoll += (TargetRoll - LeanRoll) * Blend;

	Mesh->SetRotation(Mul(QuatFromAxisAngle(Forward, LeanRoll), QuatFromAxisAngle(Right, LeanPitch)));
}
