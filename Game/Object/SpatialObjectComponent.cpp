#include "SpatialObjectComponent.h"

#include "SpatialObject.h"

SpatialObjectComponent_c::SpatialObjectComponent_c(const std::shared_ptr<SpatialObject_c>& InSpatialOwner)
	: ObjectComponent_c(InSpatialOwner)
	, SpatialOwner(InSpatialOwner)
{}

float3 SpatialObjectComponent_c::GetPosition() const
{
	const SpatialObject_c* Owner = GetSpatialOwner();
	return Owner ? Owner->GetTransform().GetPosition() : float3(0.0f);
}

float3 SpatialObjectComponent_c::GetRotation() const
{
	const SpatialObject_c* Owner = GetSpatialOwner();
	return Owner ? Owner->GetTransform().GetRotation() : float3(0.0f);
}

float SpatialObjectComponent_c::GetScale() const
{
	const SpatialObject_c* Owner = GetSpatialOwner();
	return Owner ? Owner->GetTransform().GetScale() : 0.0f;
}

matrix SpatialObjectComponent_c::GetTransform() const
{
	const SpatialObject_c* Owner = GetSpatialOwner();
	return Owner ? Owner->GetTransform().GetMatrix() : MakeMatrixIdentity();
}
