#include "Object/PropertyVisitor.h"

#include "Utility/Transform.h"

#include <Shared/FileUtils/JsonHelpers.h>
#include <Shared/FileUtils/JsonValue.h>

bool VisitTransformProperties(PropertyVisitor_c& Visitor, Transform_s& Transform)
{
	float3 Position = Transform.GetPosition();
	quat Rotation = Transform.GetRotationQuat();
	float3 Scale = Transform.GetScale();

	bool Changed = Visitor.Float3("Position", Position);
	Changed |= Visitor.Rotation("Rotation", Rotation);
	Changed |= Visitor.Scale("Scale", Scale);

	if (Changed)
	{
		Transform.Set(Position, Rotation, Scale);
	}
	return Changed;
}

bool JsonReadVisitor_c::Bool(const char* Name, bool& Value, const PropertyMeta_s& Meta)
{
	return JsonHelpers::ParseBool(Data, Name, Value);
}

bool JsonReadVisitor_c::Int(const char* Name, int32_t& Value, const PropertyMeta_s& Meta)
{
	return JsonHelpers::ParseInt(Data, Name, Value);
}

bool JsonReadVisitor_c::Float(const char* Name, float& Value, const PropertyMeta_s& Meta)
{
	return JsonHelpers::ParseFloat(Data, Name, Value);
}

bool JsonReadVisitor_c::Float3(const char* Name, float3& Value, const PropertyMeta_s& Meta)
{
	return JsonHelpers::ParseFloat3(Data, Name, Value);
}

bool JsonReadVisitor_c::Color3(const char* Name, float3& Value, const PropertyMeta_s& Meta)
{
	return JsonHelpers::ParseFloat3(Data, Name, Value);
}

bool JsonReadVisitor_c::String(const char* Name, std::string& Value, const PropertyMeta_s& Meta)
{
	return JsonHelpers::ParseString(Data, Name, Value);
}

bool JsonReadVisitor_c::Angle(const char* Name, float& Radians, const PropertyMeta_s& Meta)
{
	float Degrees = 0.0f;
	if (JsonHelpers::ParseFloat(Data, Name, Degrees))
	{
		Radians = ConvertToRadians(Degrees);
		return true;
	}
	return false;
}

bool JsonReadVisitor_c::Rotation(const char* Name, quat& Value, const PropertyMeta_s& Meta)
{
	float3 Degrees = {};
	if (JsonHelpers::ParseFloat3(Data, Name, Degrees))
	{
		Value = Normalize(QuatFromEuler(ConvertToRadians(Degrees)));
		return true;
	}
	return false;
}

bool JsonReadVisitor_c::Scale(const char* Name, float3& Value, const PropertyMeta_s& Meta)
{
	float Uniform = 1.0f;
	if (JsonHelpers::ParseFloat(Data, Name, Uniform, true))
	{
		Value = float3(Uniform);
		return true;
	}
	return JsonHelpers::ParseFloat3(Data, Name, Value);
}

bool JsonReadVisitor_c::Path(const char* Name, std::wstring& TokenisedPath, const PropertyMeta_s& Meta)
{
	return JsonHelpers::ParseWString(Data, Name, TokenisedPath);
}
