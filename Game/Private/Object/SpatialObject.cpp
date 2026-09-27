#include "Object/SpatialObject.h"

#include "Object/SpatialObjectComponent.h"

#include <Shared/FileUtils/JsonHelpers.h>
#include <Shared/FileUtils/JsonValue.h>

SpatialObject_c::SpatialObject_c(const ObjectArgs_s& Args)
	: Object_c(Args)
{

}

void SpatialObject_c::Deserialize(const JsonValue_s& Data)
{
	Object_c::Deserialize(Data);

	float3 Position = {};
	float3 RotationDegrees = {};
	float3 Scale = float3(1.0f);
	JsonHelpers::ParseFloat3(Data, "Position", Position);
	JsonHelpers::ParseFloat3(Data, "Rotation", RotationDegrees);

	float UniformScale = 1.0f;
	if (JsonHelpers::ParseFloat(Data, "Scale", UniformScale, true))
	{
		Scale = float3(UniformScale);
	}
	else
	{
		JsonHelpers::ParseFloat3(Data, "Scale", Scale);
	}

	// Euler stays the authoring format, in degrees, and converts on load.
	Transform.Set(Position, ConvertToRadians(RotationDegrees), Scale);
}

void SpatialObject_c::SetPosition(const float3& NewPosition)
{
	Transform.SetPosition(NewPosition);
	NotifyComponentsTransformed();
}

void SpatialObject_c::SetRotation(const float3& NewRotation)
{
	Transform.SetRotation(NewRotation);
	NotifyComponentsTransformed();
}

void SpatialObject_c::SetRotation(quat NewRotation)
{
	Transform.SetRotation(NewRotation);
	NotifyComponentsTransformed();
}

void SpatialObject_c::SetScale(const float3& NewScale)
{
	Transform.SetScale(NewScale);
	NotifyComponentsTransformed();
}

void SpatialObject_c::SetScale(float NewScale)
{
	Transform.SetScale(NewScale);
	NotifyComponentsTransformed();
}

void SpatialObject_c::Translate(const float3& Translation)
{
	Transform.SetPosition(Transform.GetPosition() + Translation);
	NotifyComponentsTransformed();
}

void SpatialObject_c::Rotate(quat Delta)
{
	Transform.Rotate(Delta);
	NotifyComponentsTransformed();
}

void SpatialObject_c::RotateLocal(quat Delta)
{
	Transform.RotateLocal(Delta);
	NotifyComponentsTransformed();
}

void SpatialObject_c::ResetMotion()
{
	for (SpatialObjectComponent_c* Component : GetComponents<SpatialObjectComponent_c>())
	{
		Component->ResetMotion();
	}
}

void SpatialObject_c::NotifyComponentsTransformed()
{
	for (SpatialObjectComponent_c* Component : GetComponents<SpatialObjectComponent_c>())
	{
		Component->OnTransformed();
	}
}
