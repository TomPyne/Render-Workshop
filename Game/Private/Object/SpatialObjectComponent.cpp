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

float3 SpatialObjectComponent_c::GetWorldRotation() const
{
	const matrix& World = GetWorldMatrix();

	// Every angle below is an atan2 of two elements sharing the accumulated uniform scale,
	// so the scale cancels and the rows can be used unnormalized.

	// Row 2 is (cp*sy, -sp, cp*cy), which is exactly what GetEulerFromDirection inverts.
	float3 Euler = GetEulerFromDirection(World.r[2].xyz);

	constexpr float PitchEpsilon = 1.0e-4f;
	if (fabsf(cosf(Euler.x)) > PitchEpsilon)
	{
		// m[0][1] is sr*cp and m[1][1] is cr*cp, so the pitch term divides out.
		Euler.z = atan2f(World.m[0][1], World.m[1][1]);
	}
	else
	{
		// Gimbal lock: pitch is +-90 degrees, so yaw and roll spin about the same axis.
		// Leave roll at zero and take the whole spin as yaw from the right vector.
		Euler.y = atan2f(-World.m[0][2], World.m[0][0]);
	}

	return Euler;
}