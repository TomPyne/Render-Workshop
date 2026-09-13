#include "Object/SpatialObject.h"

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
