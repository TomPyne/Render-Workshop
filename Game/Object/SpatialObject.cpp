#include "SpatialObject.h"

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
	float3 Rotation = {};
	float Scale = 1.0f;
	JsonHelpers::ParseFloat3(Data, "Position", Position);
	JsonHelpers::ParseFloat3(Data, "Rotation", Position);
	JsonHelpers::ParseFloat(Data, "Scale", Scale);
	Transform.Set(Position, Rotation, Scale);
}
