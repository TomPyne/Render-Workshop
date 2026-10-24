#include "CircleMoverComponent.h"

#include <Object/SpatialObject.h>
#include <Shared/FileUtils/JsonValue.h>

#include <cmath>

void CircleMoverComponent_c::Deserialize(const JsonValue_s& Data)
{
	Super::Deserialize(Data);

	JsonHelpers::ParseFloat(Data, "Radius", Radius);
	JsonHelpers::ParseFloat(Data, "Period", Period);
}

void CircleMoverComponent_c::OnCreate()
{
	Super::OnCreate();

	if (SpatialObject_c* Owner = dynamic_cast<SpatialObject_c*>(GetOwner()))
	{
		Centre = Owner->GetTransform().GetPosition();
	}
}

void CircleMoverComponent_c::Update(float Delta)
{
	Super::Update(Delta);

	SpatialObject_c* Owner = dynamic_cast<SpatialObject_c*>(GetOwner());
	if (!Owner || Period <= 0.0f)
		return;

	Angle = std::fmod(Angle + 2.0f * K_PI * Delta / Period, 2.0f * K_PI);
	Owner->SetPosition(Centre + float3(std::cos(Angle) * Radius, 0.0f, std::sin(Angle) * Radius));
}
