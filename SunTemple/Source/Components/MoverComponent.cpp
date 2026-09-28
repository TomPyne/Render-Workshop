#include "MoverComponent.h"

#include <Object/SpatialObject.h>

void MoverComponent_c::Deserialize(const JsonValue_s& Data)
{
	Super::Deserialize(Data);
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

		float3 Offset = Direction * sinf(CurrentPhase);
		Owner->SetPosition(StartPos + Offset);	
	}
}
