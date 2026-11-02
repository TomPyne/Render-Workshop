#pragma once

#include <SurfMath.h>

#include <cstdint>
#include <string>

struct JsonValue_s;
struct Transform_s;

struct PropertyMeta_s
{
	// Unbounded when Min == Max
	float Min = 0.0f;
	float Max = 0.0f;
	float Speed = 0.1f;
	bool ReadOnly = false;
};

// One field list per class drives level loading, the inspector and, later, saving.
// Every primitive returns true when Value changed: for a reader that means the field was present,
// for the inspector that the user edited it. Values only change when it returns true.
class PropertyVisitor_c
{
public:
	virtual ~PropertyVisitor_c() = default;

	// True while deserializing, before OnCreate, so change notifications can be skipped as OnCreate does the initial sync
	virtual bool IsLoading() const { return false; }

	virtual bool Bool(const char* Name, bool& Value, const PropertyMeta_s& Meta = {}) = 0;
	virtual bool Int(const char* Name, int32_t& Value, const PropertyMeta_s& Meta = {}) = 0;
	virtual bool Float(const char* Name, float& Value, const PropertyMeta_s& Meta = {}) = 0;
	virtual bool Float3(const char* Name, float3& Value, const PropertyMeta_s& Meta = {}) = 0;
	virtual bool Color3(const char* Name, float3& Value, const PropertyMeta_s& Meta = {}) = 0;
	virtual bool String(const char* Name, std::string& Value, const PropertyMeta_s& Meta = {}) = 0;

	// Stored in radians, authored and displayed in degrees
	virtual bool Angle(const char* Name, float& Radians, const PropertyMeta_s& Meta = {}) = 0;

	// Authored and displayed as Euler degrees
	virtual bool Rotation(const char* Name, quat& Value, const PropertyMeta_s& Meta = {}) = 0;

	// Authored as either a uniform float or a float3
	virtual bool Scale(const char* Name, float3& Value, const PropertyMeta_s& Meta = {}) = 0;

	// The tokenised path as authored, resolve with Path_s::FromTokenised
	virtual bool Path(const char* Name, std::wstring& TokenisedPath, const PropertyMeta_s& Meta = {}) = 0;
};

// Visits Position, Rotation and Scale. Returns true if any changed, and Transform is already updated.
bool VisitTransformProperties(PropertyVisitor_c& Visitor, Transform_s& Transform);

class JsonReadVisitor_c : public PropertyVisitor_c
{
public:
	JsonReadVisitor_c(const JsonValue_s& InData)
		: Data(InData)
	{}

	// Begin PropertyVisitor_c interface
	virtual bool IsLoading() const override { return true; }
	virtual bool Bool(const char* Name, bool& Value, const PropertyMeta_s& Meta = {}) override;
	virtual bool Int(const char* Name, int32_t& Value, const PropertyMeta_s& Meta = {}) override;
	virtual bool Float(const char* Name, float& Value, const PropertyMeta_s& Meta = {}) override;
	virtual bool Float3(const char* Name, float3& Value, const PropertyMeta_s& Meta = {}) override;
	virtual bool Color3(const char* Name, float3& Value, const PropertyMeta_s& Meta = {}) override;
	virtual bool String(const char* Name, std::string& Value, const PropertyMeta_s& Meta = {}) override;
	virtual bool Angle(const char* Name, float& Radians, const PropertyMeta_s& Meta = {}) override;
	virtual bool Rotation(const char* Name, quat& Value, const PropertyMeta_s& Meta = {}) override;
	virtual bool Scale(const char* Name, float3& Value, const PropertyMeta_s& Meta = {}) override;
	virtual bool Path(const char* Name, std::wstring& TokenisedPath, const PropertyMeta_s& Meta = {}) override;
	// End PropertyVisitor_c interface

private:
	const JsonValue_s& Data;
};
