#include "RotatorComponent.h"

#include <Object/SpatialObject.h>
#include <Shared/FileUtils/JsonValue.h>

void RotatorComponent_c::Deserialize(const JsonValue_s& Data)
{
	Super::Deserialize(Data);

	JsonHelpers::ParseFloat3(Data, "PivotOffset", PivotOffset);
	JsonHelpers::ParseFloat3(Data, "Axis", Axis);
	JsonHelpers::ParseFloat(Data, "DegreesPerSecond", DegreesPerSecond);
}

void RotatorComponent_c::OnCreate()
{
	Super::OnCreate();

	if (SpatialObject_c* Owner = GetSpatialOwner())
	{
		StartPos = Owner->GetTransform().GetPosition();
		StartRot = Owner->GetTransform().GetRotationQuat();
	}
}

void RotatorComponent_c::Update(float Delta)
{
	Super::Update(Delta);

	if (SpatialObject_c* Owner = GetSpatialOwner())
	{
		CurrentAngle = fmodf(CurrentAngle + Delta * ConvertToRadians(DegreesPerSecond), 2.0f * K_PI);

		const quat Spin = QuatFromAxisAngle(Axis, CurrentAngle);
		const float3 Pivot = StartPos + PivotOffset;

		Owner->SetPosition(Pivot + ::Rotate(Spin, StartPos - Pivot));
		Owner->SetRotation(Mul(StartRot, Spin));
	}
}
