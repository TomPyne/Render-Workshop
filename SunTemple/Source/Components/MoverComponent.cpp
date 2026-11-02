#include "MoverComponent.h"

#include <Object/PropertyVisitor.h>
#include <Object/SpatialObject.h>

void MoverComponent_c::VisitProperties(PropertyVisitor_c& Visitor)
{
	Super::VisitProperties(Visitor);

	Visitor.Float3("Direction", Direction);
	Visitor.Float("Distance", Distance, { .Min = 0.0f, .Max = 1000.0f });
	Visitor.Float("Speed", Speed, { .Min = 0.0f, .Max = 100.0f, .Speed = 0.01f });
}

void MoverComponent_c::OnCreate()
{
	Super::OnCreate();

	if (SpatialObject_c* Owner = GetSpatialOwner())
	{
		StartPos = GetWorldPosition();
	}
}

void MoverComponent_c::Update(float Delta)
{
	Super::Update(Delta);

	if (SpatialObject_c* Owner = GetSpatialOwner())
	{
		CurrentPhase += Delta * Speed * K_PI;

		float3 Offset = Normalize(Direction) * Distance * sinf(CurrentPhase);
		Owner->SetPosition(StartPos + Offset);	
	}
}
