#include "Object/FlyControllerComponent.h"

#include "Input/Input.h"
#include "Object/SpatialObject.h"
#include "Utility/Transform.h"

void FlyControllerComponent_c::Update(float Delta)
{
	Super::Update(Delta);

	if (!IsActiveController())
		return;

	SpatialObject_c* SpatialOwner = dynamic_cast<SpatialObject_c*>(GetOwner());

	if (!SpatialOwner)
		return;

	// The owner's rotation holds the view angles, so anything else that turns the
	// object is picked up here instead of being overwritten on the next frame.
	const float3 CurrentView = QuatToEuler(SpatialOwner->GetTransform().GetRotationQuat());

	float ViewPitch = ConvertToDegrees(CurrentView.x);
	float ViewYaw = ConvertToDegrees(CurrentView.y);

	const bool Looking = Input::IsMouseButtonDown(1);

	Input::SetMouseCaptured(Looking);

	if (Looking)
	{
		constexpr float Sensitivity = 0.25f;

		float2 MouseDelta = Input::GetMouseDelta();
		ViewPitch += MouseDelta.y * Sensitivity;
		ViewYaw += MouseDelta.x * Sensitivity;
	}

	// Yaw needs no wrapping, since it is read back from a canonical quaternion each
	// frame. The pitch clamp is a free-look usability choice.
	ViewPitch = Clamp(ViewPitch, -85.0f, 85.0f);

	const float YawRad = ConvertToRadians(ViewYaw);
	const float PitchRad = ConvertToRadians(ViewPitch);

	// Pitch about our own right axis, then yaw about world up.
	const quat Rotation = Mul(
		QuatFromAxisAngle(float3{ 1.0f, 0.0f, 0.0f }, PitchRad),
		QuatFromAxisAngle(float3{ 0.0f, 1.0f, 0.0f }, YawRad));

	const float3 Fwd = Rotate(Rotation, float3{ 0.0f, 0.0f, 1.0f });

	// Deliberately the horizontal right, so pitching does not tilt the strafe axis.
	const float3 Rgt = Normalize(Cross(float3{ 0, 1, 0 }, Fwd));

	constexpr float Speed = 5.0f;

	float MoveSpeed = Speed * Delta;

	if (Input::IsKeyDown(KeyCode_e::_SHIFT))
		MoveSpeed *= 4.0f;

	float3 TranslateDir = 0.0f;

	if (Input::IsKeyDown(KeyCode_e::_W)) TranslateDir += Fwd;
	if (Input::IsKeyDown(KeyCode_e::_S)) TranslateDir -= Fwd;

	if (Input::IsKeyDown(KeyCode_e::_D)) TranslateDir += Rgt;
	if (Input::IsKeyDown(KeyCode_e::_A)) TranslateDir -= Rgt;

	if (Input::IsKeyDown(KeyCode_e::_E)) TranslateDir.y += 1.0f;
	if (Input::IsKeyDown(KeyCode_e::_Q)) TranslateDir.y -= 1.0f;

	const float3 Translation = Normalize(TranslateDir) * MoveSpeed;

	SpatialOwner->Translate(Translation);
	SpatialOwner->SetRotation(Rotation);
}
