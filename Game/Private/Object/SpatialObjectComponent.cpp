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
	float3 RotationDegrees = {};
	float Scale = 1.0f;
	JsonHelpers::ParseFloat3(Data, "Position", Position);
	JsonHelpers::ParseFloat3(Data, "Rotation", RotationDegrees);

	if (!JsonHelpers::ParseFloat(Data, "Scale", Scale, true))
	{
		// TODO: Non uniform scale
		float3 NonUniformScale = float3(1.0f);
		if (JsonHelpers::ParseFloat3(Data, "Scale", NonUniformScale))
		{
			LOGWARNING("[SpatialObjectComponent_c::Deserialize] Attempted to parse a non-uniform scale");
			Scale = NonUniformScale.x;
		}
	}

	// Euler stays the authoring format, in degrees, and converts on load.
	Transform.Set(Position, ConvertToRadians(RotationDegrees), Scale);
}

const matrix& SpatialObjectComponent_c::GetWorldMatrix() const
{
	const SpatialObject_c* Owner = GetSpatialOwner();
	WorldMatrix = Owner ? Transform.GetMatrix() * Owner->GetTransform().GetMatrix() : Transform.GetMatrix();
	return WorldMatrix;
}

quat SpatialObjectComponent_c::GetWorldRotation() const
{
	const SpatialObject_c* Owner = GetSpatialOwner();
	return Owner
		? Mul(Transform.GetRotationQuat(), Owner->GetTransform().GetRotationQuat())
		: Transform.GetRotationQuat();
}