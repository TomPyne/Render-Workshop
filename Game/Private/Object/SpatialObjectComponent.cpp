#include "Object/SpatialObjectComponent.h"

#include "Object/PropertyVisitor.h"
#include "Object/SpatialObject.h"

#include <Shared/FileUtils/JsonValue.h>
#include <Shared/Logging/Logging.h>

SpatialObjectComponent_c::SpatialObjectComponent_c(const ObjectComponentArgs_s& Args)
	: ObjectComponent_c(Args)
{
	SpatialOwner = std::dynamic_pointer_cast<SpatialObject_c>(Args.Owner.lock());
	ENSUREMSG(!SpatialOwner.expired(), "Failed to correctly create component for spatial object. Owner is not a SpatialObject_c.");
}

void SpatialObjectComponent_c::VisitProperties(PropertyVisitor_c& Visitor)
{
	Super::VisitProperties(Visitor);

	if (VisitTransformProperties(Visitor, Transform) && !Visitor.IsLoading())
	{
		OnTransformed();
	}
}

void SpatialObjectComponent_c::SetPosition(const float3& NewPosition) 
{ 
	Transform.SetPosition(NewPosition);
	OnTransformed();
}

void SpatialObjectComponent_c::SetRotation(const float3& NewRotation)
{ 
	Transform.SetRotation(NewRotation);
	OnTransformed();
}

void SpatialObjectComponent_c::SetRotation(quat NewRotation)
{ 
	Transform.SetRotation(NewRotation);
	OnTransformed();
}

void SpatialObjectComponent_c::SetScale(const float3& NewScale) 
{ 
	Transform.SetScale(NewScale);
	OnTransformed();
}

void SpatialObjectComponent_c::SetScale(float NewScale) 
{ 
	Transform.SetScale(NewScale);
	OnTransformed();
}

void SpatialObjectComponent_c::Rotate(quat Delta) 
{ 
	Transform.Rotate(Delta);
	OnTransformed();
}

void SpatialObjectComponent_c::RotateLocal(quat Delta) 
{ 
	Transform.RotateLocal(Delta);
	OnTransformed();
}

const matrix& SpatialObjectComponent_c::GetWorldMatrix() const
{
	const SpatialObject_c* Owner = GetSpatialOwner();
	WorldMatrix = Owner ? Transform.GetMatrix() * Owner->GetWorldMatrix() : Transform.GetMatrix();
	return WorldMatrix;
}

quat SpatialObjectComponent_c::GetWorldRotation() const
{
	const SpatialObject_c* Owner = GetSpatialOwner();
	if (!Owner)
	{
		return Transform.GetRotationQuat();
	}

	ENSUREMSG(Owner->GetTransform().IsUniformScale(), "[SpatialObjectComponent_c::GetWorldRotation] Owner has non-uniform scale, the returned rotation does not match GetWorldMatrix.");

	return Mul(Transform.GetRotationQuat(), Owner->GetWorldRotation());
}