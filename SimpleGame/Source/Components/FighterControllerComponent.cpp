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

	// MakeMatrixRotationFromVector applies Z, then X, then Y, so writing a roll into
	// Rotation.x turns the fighter about the world X axis rather than about its own
	// nose, which mirrors its pitch. Build the rotation we want - roll about the model
	// X axis, then pitch about model Z - and decompose it back into an euler triple.
	float3 MakeFighterRotation(float Pitch, float Roll)
	{
		const float CosPitch = cosf(Pitch);
		const float SinPitch = sinf(Pitch);
		const float CosRoll = cosf(Roll);
		const float SinRoll = sinf(Roll);

		return float3
		{
			asinf(Clamp(SinRoll * CosPitch, -1.0f, 1.0f)),
			atan2f(SinRoll * SinPitch, CosRoll),
			atan2f(SinPitch, CosRoll * CosPitch)
		};
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

		FighterMeshComp->SetRotation(MakeFighterRotation(Pitch, (Pitch < 0.0f ? -K_PI : K_PI) * RollAlpha));
	}
}
