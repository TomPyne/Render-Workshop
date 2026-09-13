#include "Object/SpatialObject.h"

#include <Shared/FileUtils/JsonHelpers.h>
#include <Shared/FileUtils/JsonValue.h>
#include <Shared/Logging/Logging.h>

SpatialObject_c::SpatialObject_c(const ObjectArgs_s& Args)
	: Object_c(Args)
{

}

void SpatialObject_c::Deserialize(const JsonValue_s& Data)
{
	Object_c::Deserialize(Data);

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
			LOGWARNING("[SpatialObject_c::Deserialize] Attempted to parse a non-uniform scale");
			Scale = NonUniformScale.x;
		}
	}

	// Euler stays the authoring format, in degrees, and converts on load.
	Transform.Set(Position, ConvertToRadians(RotationDegrees), Scale);
}
