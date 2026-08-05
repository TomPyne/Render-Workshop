#include "JsonValue.h"

#include "Logging/Logging.h"
#include "StringUtils/StringUtils.h"

#include <fstream>

bool LoadJsonFromFile(const std::wstring& Path, Json_t& OutJson)
{
	std::ifstream AssetFile(WideToNarrow(Path).c_str());

	if (!AssetFile.is_open())
	{
		LOGERROR("[LoadJsonFromFile] Failed to open file %S", Path.c_str());
		return false;
	}

	constexpr bool AllowExceptions = false;
	OutJson = Json_t::parse(AssetFile, nullptr, AllowExceptions);

	if (OutJson.is_discarded())
	{
		LOGERROR("[LoadJsonFromFile] Failed to parse json in file %S", Path.c_str());
		return false;
	}

	return true;
}