#include "MoverComponent.h"

#include <Object/SpatialObject.h>
#include <Shared/FileUtils/JsonValue.h>

void MoverComponent_c::Deserialize(const JsonValue_s& Data)
{
	Super::Deserialize(Data);

	JsonHelpers::ParseFloat3(Data, "Direction", Direction);
	JsonHelpers::ParseFloat(Data, "Distance", Distance);
	JsonHelpers::ParseFloat(Data, "Speed", Speed);
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
