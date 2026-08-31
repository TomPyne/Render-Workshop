#include "FighterControllerComponent.h"

#include <Input/Input.h>
#include <Object/Object.h>
#include <Object/SpatialObject.h>
#include <Object/MeshComponent.h>
#include <Utility/SharedPtr.h>

namespace
{
	float WrapAngle(float Angle)
	{
		Angle = fmodf(Angle + K_PI, 2.0f * K_PI);
		return (Angle < 0.0f ? Angle + 2.0f * K_PI : Angle) - K_PI;
	}

	float SmoothStep(float Edge0, float Edge1, float Value)
	{
		const float T = Clamp((Value - Edge0) / (Edge1 - Edge0), 0.0f, 1.0f);
		return T * T * (3.0f - 2.0f * T);
	}
}

void FighterControllerComponent_c::OnCreate()
{
	Super::OnCreate();

	if (GetOwner())
	{
		SpatialOwner = SharedCast<SpatialObject_c>(GetOwner()).get();
	}
}

void FighterControllerComponent_c::Update(float Delta)
{
	Super::Update(Delta);

	if (!SpatialOwner)
		return;

	if (!IsActiveController())
		return;

	if (FighterMeshComp == nullptr)
	{
		SpatialOwner->ForEachComponentType<MeshComponent_c>([this](MeshComponent_c* InMeshComp)
		{
			FighterMeshComp = InMeshComp;
			return false;
		});
	}	

	float3 Position = SpatialOwner->GetTransform().GetPosition();
	
	if (Input::IsKeyDown(KeyCode_e::_W)) Position.y += 1.0f * Delta;
	if (Input::IsKeyDown(KeyCode_e::_S)) Position.y -= 1.0f * Delta;

	SpatialOwner->SetPosition(Position);

	if (Input::IsKeyDown(KeyCode_e::_A)) Pitch += PitchRate * Delta;
	if (Input::IsKeyDown(KeyCode_e::_D)) Pitch -= PitchRate * Delta;

	Pitch = WrapAngle(Pitch);

	if (FighterMeshComp)
	{
		// Past vertical the fighter is heading the other way, so it needs to be rolled
		// 180 degrees to keep its belly pointing at the ground. Blend that flip across
		// the near-vertical band, where the roll makes least difference to the belly.
		const float RollAlpha = SmoothStep(ConvertToRadians(RollStartPitchDegrees), ConvertToRadians(RollEndPitchDegrees), fabsf(Pitch));
		const float Roll = (Pitch < 0.0f ? -K_PI : K_PI) * RollAlpha;

		// Roll about the model nose, then pitch about the model Z axis.
		FighterMeshComp->SetRotation(Mul(
			QuatFromAxisAngle(float3{ 1.0f, 0.0f, 0.0f }, Roll),
			QuatFromAxisAngle(float3{ 0.0f, 0.0f, 1.0f }, Pitch)));
	}
}
