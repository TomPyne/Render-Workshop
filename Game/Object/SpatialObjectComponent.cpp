#include "SpatialObjectComponent.h"

#include "SpatialObject.h"

#include <Shared/Logging/Logging.h>

SpatialObjectComponent_c::SpatialObjectComponent_c(const ObjectComponentArgs_s& Args)
	: ObjectComponent_c(Args)
{
	SpatialOwner = std::dynamic_pointer_cast<SpatialObject_c>(Args.Owner.lock());
	ENSUREMSG(!SpatialOwner.expired(), "Failed to correctly create component for spatial object. Owner is not a SpatialObject_c.");
}

const Transform_s& SpatialObjectComponent_c::GetTransform() const
{
	static const Transform_s DefaultTransform = {};
	const SpatialObject_c* Owner = GetSpatialOwner();
	return Owner ? Owner->GetTransform() : DefaultTransform;
}