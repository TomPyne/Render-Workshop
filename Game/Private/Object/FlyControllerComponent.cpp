#include "Object/FlyControllerComponent.h"

#include "Input/Input.h"
#include "Object/SpatialObject.h"
#include "Utility/Transform.h"

void FlyControllerComponent_c::Update(float Delta)
{
	Super::Update(Delta);

	if (!IsActiveController())
		return;

	const bool Looking = Input::IsMouseButtonDown(1);

	Input::SetMouseCaptured(Looking);

	if (Looking)
	{
		constexpr float Sensitivity = 0.25f;

		float2 MouseDelta = Input::GetMouseDelta();
		ViewPitch += MouseDelta.y * Sensitivity;
		ViewYaw += MouseDelta.x * Sensitivity;
	}

	ViewYaw = fmodf(ViewYaw, 360.0f);
	ViewPitch = Clamp(ViewPitch, -85.0f, 85.0f);

	const float YawRad = ConvertToRadians(ViewYaw);
	const float PitchRad = ConvertToRadians(ViewPitch);

	const float3 Rotation = float3{ PitchRad, YawRad, 0.0f };

	const float3 Fwd = GetDirectionFromEuler(Rotation);
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

	if (SpatialObject_c* SpatialOwner = dynamic_cast<SpatialObject_c*>(GetOwner()))
	{
		SpatialOwner->Translate(Translation);
		SpatialOwner->SetRotation(Rotation);
	}
}
