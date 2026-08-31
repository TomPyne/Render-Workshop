#include "Object/SpatialObjectComponent.h"

#include "Object/SpatialObject.h"

#include <Shared/FileUtils/JsonValue.h>
#include <Shared/Logging/Logging.h>

SpatialObjectComponent_c::SpatialObjectComponent_c(const ObjectComponentArgs_s& Args)
	: ObjectComponent_c(Args)
{
	SpatialOwner = std::dynamic_pointer_cast<SpatialObject_c>(Args.Owner.lock());
	ENSUREMSG(!SpatialOwner.expired(), "Failed to correctly create component for spatial object. Owner is not a SpatialObject_c.");
}

void SpatialObjectComponent_c::Deserialize(const JsonValue_s& Data)
{
	ObjectComponent_c::Deserialize(Data);
	
	float3 Position = {};
	float3 Rotation = {};
	float Scale = 1.0f;
	JsonHelpers::ParseFloat3(Data, "Position", Position);
	JsonHelpers::ParseFloat3(Data, "Rotation", Rotation);
	JsonHelpers::ParseFloat(Data, "Scale", Scale);
	Transform.Set(Position, Rotation, Scale);
}

const matrix& SpatialObjectComponent_c::GetWorldMatrix() const
{
	const SpatialObject_c* Owner = GetSpatialOwner();
	WorldMatrix = Owner ? Transform.GetMatrix() * Owner->GetTransform().GetMatrix() : Transform.GetMatrix();
	return WorldMatrix;
}