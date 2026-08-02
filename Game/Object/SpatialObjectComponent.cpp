#include "SpatialObjectComponent.h"

#include "SpatialObject.h"

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
	
	// TODO: once we have component transforms
	
	//float3 Position = {};
	//float3 Rotation = {};
	//float Scale = 1.0f;
	//JsonHelpers::ParseFloat3(Data, "Position", Position);
	//JsonHelpers::ParseFloat3(Data, "Rotation", Position);
	//JsonHelpers::ParseFloat(Data, "Scale", Scale);
	//Transform.Set(Position, Rotation, Scale);
}

const Transform_s& SpatialObjectComponent_c::GetTransform() const
{
	static const Transform_s DefaultTransform = {};
	const SpatialObject_c* Owner = GetSpatialOwner();
	return Owner ? Owner->GetTransform() : DefaultTransform;
}