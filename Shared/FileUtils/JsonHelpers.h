#pragma once

#include <SurfMath.h>

#include <cstdint>
#include <string>

struct JsonValue_s;

namespace JsonHelpers
{
	// Every Parse leaves Out untouched and returns false if Field is absent, so json only needs
	// to author the fields that override a default.
	// A field that is present but the wrong type or malformed logs a warning.

	bool ParseBool(const JsonValue_s& Node, const char* Field, bool& Out);

	bool ParseInt(const JsonValue_s& Node, const char* Field, int32_t& Out);
	bool ParseInt(const JsonValue_s& Node, const char* Field, uint32_t& Out);
	bool ParseInt(const JsonValue_s& Node, const char* Field, int64_t& Out);
	bool ParseInt(const JsonValue_s& Node, const char* Field, uint64_t& Out);

	bool ParseFloat(const JsonValue_s& Node, const char* Field, float& Out);

	bool ParseString(const JsonValue_s& Node, const char* Field, std::string& Out);
	bool ParseWString(const JsonValue_s& Node, const char* Field, std::wstring& Out);

	// Vectors are authored as comma separated strings, e.g. "0, 5, -10", not json arrays
	bool ParseFloat2(const JsonValue_s& Node, const char* Field, float2& Out);
	bool ParseFloat3(const JsonValue_s& Node, const char* Field, float3& Out);
	bool ParseFloat4(const JsonValue_s& Node, const char* Field, float4& Out);
}
