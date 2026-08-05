#include "JsonHelpers.h"

#include "JsonValue.h"

#include "Logging/Logging.h"
#include "StringUtils/StringUtils.h"

#include <cstdlib>

namespace JsonHelpers
{

// Returns null if the field is absent, which is not an error - callers treat it as "unset"
static const Json_t* FindField(const JsonValue_s& Node, const char* Field)
{
	if (!Node.Json.is_object())
	{
		LOGWARNING("[Json] Node containing field '%s' is not an object", Field);
		return nullptr;
	}

	auto It = Node.Json.find(Field);
	if (It == Node.Json.end())
	{
		return nullptr;
	}

	return &(*It);
}

bool ParseBool(const JsonValue_s& Node, const char* Field, bool& Out)
{
	const Json_t* Value = FindField(Node, Field);
	if (!Value)
	{
		return false;
	}

	if (!Value->is_boolean())
	{
		LOGWARNING("[Json] Field '%s' is not a boolean", Field);
		return false;
	}

	Out = Value->get<bool>();
	return true;
}

bool ParseInt(const JsonValue_s& Node, const char* Field, int64_t& Out)
{
	const Json_t* Value = FindField(Node, Field);
	if (!Value)
	{
		return false;
	}

	// is_number_integer covers both signed and unsigned, so check unsigned first
	if (Value->is_number_unsigned())
	{
		const uint64_t Unsigned = Value->get<uint64_t>();
		if (Unsigned > static_cast<uint64_t>(INT64_MAX))
		{
			LOGWARNING("[Json] Field '%s' does not fit in a signed 64 bit integer", Field);
			return false;
		}

		Out = static_cast<int64_t>(Unsigned);
		return true;
	}

	if (!Value->is_number_integer())
	{
		LOGWARNING("[Json] Field '%s' is not an integer", Field);
		return false;
	}

	Out = Value->get<int64_t>();
	return true;
}

bool ParseInt(const JsonValue_s& Node, const char* Field, uint64_t& Out)
{
	const Json_t* Value = FindField(Node, Field);
	if (!Value)
	{
		return false;
	}

	if (!Value->is_number_integer())
	{
		LOGWARNING("[Json] Field '%s' is not an integer", Field);
		return false;
	}

	if (!Value->is_number_unsigned() && Value->get<int64_t>() < 0)
	{
		LOGWARNING("[Json] Field '%s' is negative, expected an unsigned integer", Field);
		return false;
	}

	Out = Value->get<uint64_t>();
	return true;
}

bool ParseInt(const JsonValue_s& Node, const char* Field, int32_t& Out)
{
	int64_t Wide = 0;
	if (!ParseInt(Node, Field, Wide))
	{
		return false;
	}

	if (Wide < INT32_MIN || Wide > INT32_MAX)
	{
		LOGWARNING("[Json] Field '%s' does not fit in a signed 32 bit integer", Field);
		return false;
	}

	Out = static_cast<int32_t>(Wide);
	return true;
}

bool ParseInt(const JsonValue_s& Node, const char* Field, uint32_t& Out)
{
	uint64_t Wide = 0;
	if (!ParseInt(Node, Field, Wide))
	{
		return false;
	}

	if (Wide > UINT32_MAX)
	{
		LOGWARNING("[Json] Field '%s' does not fit in an unsigned 32 bit integer", Field);
		return false;
	}

	Out = static_cast<uint32_t>(Wide);
	return true;
}

bool ParseFloat(const JsonValue_s& Node, const char* Field, float& Out)
{
	const Json_t* Value = FindField(Node, Field);
	if (!Value)
	{
		return false;
	}

	if (!Value->is_number())
	{
		LOGWARNING("[Json] Field '%s' is not a number", Field);
		return false;
	}

	Out = Value->get<float>();
	return true;
}

bool ParseString(const JsonValue_s& Node, const char* Field, std::string& Out)
{
	const Json_t* Value = FindField(Node, Field);
	if (!Value)
	{
		return false;
	}

	if (!Value->is_string())
	{
		LOGWARNING("[Json] Field '%s' is not a string", Field);
		return false;
	}

	Out = Value->get<std::string>();
	return true;
}

bool ParseWString(const JsonValue_s& Node, const char* Field, std::wstring& Out)
{
	std::string Narrow;
	if (!ParseString(Node, Field, Narrow))
	{
		return false;
	}

	Out = NarrowToWide(Narrow);
	return true;
}

// Parses Count comma separated floats from a string field, e.g. "0, 5, -10".
// Out is only written on success, so a malformed field leaves the caller's value alone.
static bool ParseFloatComponents(const JsonValue_s& Node, const char* Field, float* Out, int32_t Count)
{
	std::string Value;
	if (!ParseString(Node, Field, Value))
	{
		return false;
	}

	float Components[4] = {}; // Count is never more than 4
	const char* Cursor = Value.c_str();

	for (int32_t Index = 0; Index < Count; Index++)
	{
		char* End = nullptr;
		Components[Index] = strtof(Cursor, &End);

		if (End == Cursor)
		{
			LOGWARNING("[Json] Field '%s' expects %d comma separated floats, got '%s'", Field, Count, Value.c_str());
			return false;
		}

		Cursor = End;

		while (*Cursor == ' ' || *Cursor == '\t')
		{
			Cursor++;
		}

		if (Index < Count - 1)
		{
			if (*Cursor != ',')
			{
				LOGWARNING("[Json] Field '%s' expects %d comma separated floats, got '%s'", Field, Count, Value.c_str());
				return false;
			}

			Cursor++;
		}
	}

	for (int32_t Index = 0; Index < Count; Index++)
	{
		Out[Index] = Components[Index];
	}

	// Succeeds, but the author likely made a mistake worth addressing
	if (*Cursor != '\0')
	{
		LOGWARNING("[Json] Field '%s' has trailing content after %d floats, got '%s'", Field, Count, Value.c_str());
	}

	return true;
}

bool ParseFloat2(const JsonValue_s& Node, const char* Field, float2& Out)
{
	float Components[2] = {};
	if (!ParseFloatComponents(Node, Field, Components, 2))
	{
		return false;
	}

	Out = float2(Components[0], Components[1]);
	return true;
}

bool ParseFloat3(const JsonValue_s& Node, const char* Field, float3& Out)
{
	float Components[3] = {};
	if (!ParseFloatComponents(Node, Field, Components, 3))
	{
		return false;
	}

	Out = float3(Components[0], Components[1], Components[2]);
	return true;
}

bool ParseFloat4(const JsonValue_s& Node, const char* Field, float4& Out)
{
	float Components[4] = {};
	if (!ParseFloatComponents(Node, Field, Components, 4))
	{
		return false;
	}

	Out = float4(Components[0], Components[1], Components[2], Components[3]);
	return true;
}

uint64_t Hash(const JsonValue_s& Node)
{
	std::vector<uint8_t> Bytes = Json_t::to_cbor(Node.Json);

	uint64_t Hash = 0xcbf29ce484222325ull; // FNV-1a 64
	for (uint8_t Byte : Bytes)
	{
		Hash ^= Byte;
		Hash *= 0x100000001b3ull;
	}
	return Hash;
}

}
