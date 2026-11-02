#include "Editor/ImGuiPropertyVisitor.h"

#include <RenderImGui/imgui/imgui.h>
#include <Shared/StringUtils/StringUtils.h>

#include <algorithm>
#include <cstring>

namespace
{
	// Enough decimals that a drag at the given speed visibly changes the value
	const char* GetFloatFormat(float Speed)
	{
		if (Speed < 0.001f)
			return "%.5f";
		if (Speed < 0.01f)
			return "%.4f";
		return "%.3f";
	}

	ImGuiSliderFlags GetSliderFlags(const PropertyMeta_s& Meta)
	{
		return Meta.Min < Meta.Max ? ImGuiSliderFlags_AlwaysClamp : ImGuiSliderFlags_None;
	}

	struct ReadOnlyScope_s
	{
		ReadOnlyScope_s(const PropertyMeta_s& Meta)
			: ReadOnly(Meta.ReadOnly)
		{
			if (ReadOnly)
			{
				ImGui::BeginDisabled();
			}
		}

		~ReadOnlyScope_s()
		{
			if (ReadOnly)
			{
				ImGui::EndDisabled();
			}
		}

		bool ReadOnly;
	};

	// No imgui_stdlib in RenderImGui, so text goes through a fixed buffer
	bool InputString(const char* Name, std::string& Value, ImGuiInputTextFlags Flags)
	{
		char Buffer[512];
		const size_t Length = std::min(Value.size(), sizeof(Buffer) - 1);
		std::memcpy(Buffer, Value.data(), Length);
		Buffer[Length] = '\0';

		if (ImGui::InputText(Name, Buffer, sizeof(Buffer), Flags))
		{
			Value = Buffer;
			return true;
		}
		return false;
	}
}

bool ImGuiPropertyVisitor_c::Bool(const char* Name, bool& Value, const PropertyMeta_s& Meta)
{
	ReadOnlyScope_s ReadOnly(Meta);
	return ImGui::Checkbox(Name, &Value);
}

bool ImGuiPropertyVisitor_c::Int(const char* Name, int32_t& Value, const PropertyMeta_s& Meta)
{
	ReadOnlyScope_s ReadOnly(Meta);
	return ImGui::DragInt(Name, &Value, std::max(Meta.Speed, 0.1f), static_cast<int>(Meta.Min), static_cast<int>(Meta.Max), "%d", GetSliderFlags(Meta));
}

bool ImGuiPropertyVisitor_c::Float(const char* Name, float& Value, const PropertyMeta_s& Meta)
{
	ReadOnlyScope_s ReadOnly(Meta);
	return ImGui::DragFloat(Name, &Value, Meta.Speed, Meta.Min, Meta.Max, GetFloatFormat(Meta.Speed), GetSliderFlags(Meta));
}

bool ImGuiPropertyVisitor_c::Float3(const char* Name, float3& Value, const PropertyMeta_s& Meta)
{
	ReadOnlyScope_s ReadOnly(Meta);
	return ImGui::DragFloat3(Name, &Value.x, Meta.Speed, Meta.Min, Meta.Max, GetFloatFormat(Meta.Speed), GetSliderFlags(Meta));
}

bool ImGuiPropertyVisitor_c::Color3(const char* Name, float3& Value, const PropertyMeta_s& Meta)
{
	ReadOnlyScope_s ReadOnly(Meta);
	return ImGui::ColorEdit3(Name, &Value.x, ImGuiColorEditFlags_Float);
}

bool ImGuiPropertyVisitor_c::String(const char* Name, std::string& Value, const PropertyMeta_s& Meta)
{
	ReadOnlyScope_s ReadOnly(Meta);
	return InputString(Name, Value, ImGuiInputTextFlags_None);
}

bool ImGuiPropertyVisitor_c::Angle(const char* Name, float& Radians, const PropertyMeta_s& Meta)
{
	ReadOnlyScope_s ReadOnly(Meta);

	float Degrees = ConvertToDegrees(Radians);
	if (ImGui::DragFloat(Name, &Degrees, Meta.Speed, Meta.Min, Meta.Max, GetFloatFormat(Meta.Speed), GetSliderFlags(Meta)))
	{
		Radians = ConvertToRadians(Degrees);
		return true;
	}
	return false;
}

bool ImGuiPropertyVisitor_c::Rotation(const char* Name, quat& Value, const PropertyMeta_s& Meta)
{
	ReadOnlyScope_s ReadOnly(Meta);

	const ImGuiID Id = ImGui::GetID(Name);
	const bool Editing = RotationState.ActiveId == Id;

	float3 Degrees = Editing ? RotationState.Degrees : ConvertToDegrees(QuatToEuler(Value));
	const bool Changed = ImGui::DragFloat3(Name, &Degrees.x, 0.5f, 0.0f, 0.0f, "%.2f");

	if (Changed)
	{
		RotationState.ActiveId = Id;
		RotationState.Degrees = Degrees;
		Value = Normalize(QuatFromEuler(ConvertToRadians(Degrees)));
	}
	else if (Editing && !ImGui::IsItemActive())
	{
		RotationState.ActiveId = 0;
	}
	return Changed;
}

bool ImGuiPropertyVisitor_c::Scale(const char* Name, float3& Value, const PropertyMeta_s& Meta)
{
	ReadOnlyScope_s ReadOnly(Meta);
	return ImGui::DragFloat3(Name, &Value.x, 0.01f, 0.0f, 0.0f, "%.3f");
}

bool ImGuiPropertyVisitor_c::Path(const char* Name, std::wstring& TokenisedPath, const PropertyMeta_s& Meta)
{
	ReadOnlyScope_s ReadOnly(Meta);

	// Only on enter, as each change requests the asset
	std::string Narrow = WideToNarrow(TokenisedPath);
	if (InputString(Name, Narrow, ImGuiInputTextFlags_EnterReturnsTrue))
	{
		TokenisedPath = NarrowToWide(Narrow);
		return true;
	}
	return false;
}
