#include "RotatorComponent.h"

#include <Object/PropertyVisitor.h>
#include <Object/SpatialObject.h>

void RotatorComponent_c::VisitProperties(PropertyVisitor_c& Visitor)
{
	Super::VisitProperties(Visitor);

	Visitor.Float3("PivotOffset", PivotOffset);
	Visitor.Float3("Axis", Axis);
	Visitor.Float("DegreesPerSecond", DegreesPerSecond, { .Min = -720.0f, .Max = 720.0f, .Speed = 1.0f });
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
