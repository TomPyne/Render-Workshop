#pragma once

#include <json.hpp>

// Wraps a json value so other headers can forward declare it, keeping json.hpp out of the
// global include graph. nlohmann::json is an alias for basic_json<> living in an inline
// versioned namespace, so it cannot be forward declared by hand.
// Only ever include this from a .cpp.
struct JsonValue_s
{
	JsonValue_s(const nlohmann::json& InJson) // Implicit, so call sites can pass json directly
		: Json(InJson)
	{}

	const nlohmann::json& Json;
};
