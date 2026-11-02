#pragma once

#include "Object/PropertyVisitor.h"

#include <cstdint>

// Lives across frames, as converting the quat back to Euler every frame while dragging makes the angles flip
struct RotationEditState_s
{
	uint32_t ActiveId = 0;
	float3 Degrees = {};
};

// Draws each property as an ImGui widget. Angle meta Min, Max and Speed are in degrees.
class ImGuiPropertyVisitor_c : public PropertyVisitor_c
{
public:
	ImGuiPropertyVisitor_c(RotationEditState_s& InRotationState)
		: RotationState(InRotationState)
	{}

	// Begin PropertyVisitor_c interface
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
	RotationEditState_s& RotationState;
};
