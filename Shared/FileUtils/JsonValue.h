#pragma once

#include "FileUtils/JsonHelpers.h"

#include <json.hpp>

using Json_t = nlohmann::json;

// Wraps a json value so other headers can forward declare it, keeping json.hpp out of the
// global include graph. nlohmann::json is an alias for basic_json<> living in an inline
// versioned namespace, so it cannot be forward declared by hand.
// Only ever include this from a .cpp.
struct JsonValue_s
{
	JsonValue_s(const Json_t& InJson) // Implicit, so call sites can pass json directly
		: Json(InJson)
	{}

	const Json_t& Json;

	// Lazy compute and cache of hash
	inline uint64_t GetHash() const
	{
		if (Hash == -1)
		{
			std::vector<uint8_t> Bytes = Json_t::to_cbor(Json);

			Hash = 0xcbf29ce484222325ull; // FNV-1a 64
			for (uint8_t Byte : Bytes)
			{
				Hash ^= Byte;
				Hash *= 0x100000001b3ull;
			}
		}
		return Hash;
	}
private:
	mutable uint64_t Hash = -1;
};

bool LoadJsonFromFile(const std::wstring& Path, Json_t& OutJson);