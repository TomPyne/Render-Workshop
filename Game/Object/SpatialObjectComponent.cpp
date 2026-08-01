#include "SpatialObjectComponent.h"

#include "SpatialObject.h"

SpatialObjectComponent_c::SpatialObjectComponent_c(const std::shared_ptr<SpatialObject_c>& InSpatialOwner)
	: ObjectComponent_c(InSpatialOwner)
	, SpatialOwner(InSpatialOwner)
{}

const Transform_s& SpatialObjectComponent_c::GetTransform() const
{
	static const Transform_s DefaultTransform = {};
	const SpatialObject_c* Owner = GetSpatialOwner();
	return Owner ? Owner->GetTransform() : DefaultTransform;
}