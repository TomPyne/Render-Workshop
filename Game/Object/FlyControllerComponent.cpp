#include "FlyControllerComponent.h"

#include "Game/Input/Input.h"
#include "Game/Object/SpatialObject.h"
#include "Game/Utility/Transform.h"

void FlyControllerComponent_c::Update(float Delta)
{
	if (Input::IsMouseButtonDown(1))
	{
		float2 MouseDelta = Input::GetMouseDelta();
		ViewPitch -= MouseDelta.y * 25.0f * Delta;
		ViewYaw -= MouseDelta.x * 25.0f * Delta;
	}

	float3 Translation = { 0.0f };

	float3 Fwd = LookDir;
	float3 Rgt = Cross(float3{ 0, 1, 0 }, LookDir);

	constexpr float Speed = 5.0f;

	float MoveSpeed = Speed * Delta;

	float3 TranslateDir = 0.0f;

	if (Input::IsKeyDown(KeyCode_e::_W)) TranslateDir += Fwd;
	if (Input::IsKeyDown(KeyCode_e::_S)) TranslateDir -= Fwd;

	if (Input::IsKeyDown(KeyCode_e::_D)) TranslateDir += Rgt;
	if (Input::IsKeyDown(KeyCode_e::_A)) TranslateDir -= Rgt;

	if (Input::IsKeyDown(KeyCode_e::_SHIFT))
		MoveSpeed *= 4.0f;

	Translation = Normalize(TranslateDir) * MoveSpeed;

	if (Input::IsKeyDown(KeyCode_e::_E)) Translation.y += MoveSpeed;
	if (Input::IsKeyDown(KeyCode_e::_Q)) Translation.y -= MoveSpeed;

	if (ViewYaw > 360.0f)
		ViewYaw -= 360.0f;

	if (ViewYaw < -360.0f)
		ViewYaw += 360.0f;

	ViewPitch = Clamp(ViewPitch, -85.0f, 85.0f);

	float YawRad = ConvertToRadians(ViewYaw);
	float PitchRad = ConvertToRadians(ViewPitch);

	float CosPitch = cosf(PitchRad);

	LookDir = float3{ cosf(YawRad) * CosPitch, sinf(PitchRad), sinf(YawRad) * CosPitch };

	if (SpatialObject_c* SpatialOwner = dynamic_cast<SpatialObject_c*>(GetOwner()))
	{
		SpatialOwner->Translate(Translation);
		SpatialOwner->SetRotation(float3{ PitchRad, YawRad, 0.0f });
	}
}
