#include "Level.h"

#include "Game/Space/Space.h"

#include <Shared/Logging/Logging.h>
#include <Shared/StringUtils/StringUtils.h>

#include <json.hpp>
#include <fstream>

struct LevelData_s
{
	struct ComponentData_s
	{
		std::wstring Class;

		// The whole component node, for the component to parse its own fields from
		nlohmann::json Data;
	};

	struct ObjectData_s
	{
		std::wstring Class;
		float3 Position = float3(0.0f);
		std::vector<ComponentData_s> Components;

		// The whole object node, for the object to parse its own fields from
		nlohmann::json Data;
	};

	int32_t Version = 0;
	std::vector<ObjectData_s> Objects;
};

namespace
{

// Reads an optional string field, leaving Out untouched if the field is absent
bool TryParseString(const nlohmann::json& Node, const char* Field, std::wstring& Out)
{
	auto It = Node.find(Field);
	if (It == Node.end())
	{
		return false;
	}

	if (!It->is_string())
	{
		LOGWARNING("[Level] Field '%s' is not a string", Field);
		return false;
	}

	Out = NarrowToWide(It->get<std::string>());
	return true;
}

// Positions are authored as "x, y, z" strings rather than json arrays
bool TryParseFloat3(const nlohmann::json& Node, const char* Field, float3& Out)
{
	auto It = Node.find(Field);
	if (It == Node.end())
	{
		return false;
	}

	if (!It->is_string())
	{
		LOGWARNING("[Level] Field '%s' is not a string", Field);
		return false;
	}

	const std::string Value = It->get<std::string>();

	float Components[3] = { 0.0f, 0.0f, 0.0f };
	const char* Cursor = Value.c_str();

	for (int32_t Index = 0; Index < 3; Index++)
	{
		char* End = nullptr;
		Components[Index] = strtof(Cursor, &End);

		if (End == Cursor)
		{
			LOGWARNING("[Level] Field '%s' expects 3 comma separated floats, got '%s'", Field, Value.c_str());
			return false;
		}

		Cursor = End;

		while (*Cursor == ' ' || *Cursor == '\t')
		{
			Cursor++;
		}

		if (Index < 2)
		{
			if (*Cursor != ',')
			{
				LOGWARNING("[Level] Field '%s' expects 3 comma separated floats, got '%s'", Field, Value.c_str());
				return false;
			}

			Cursor++;
		}
	}

	Out = float3(Components[0], Components[1], Components[2]);
	return true;
}

bool ParseComponent(const nlohmann::json& Node, LevelData_s::ComponentData_s& Out)
{
	if (!Node.is_object())
	{
		LOGWARNING("[Level] Component entry is not an object");
		return false;
	}

	if (!TryParseString(Node, "Class", Out.Class) || Out.Class.empty())
	{
		LOGWARNING("[Level] Component entry requires a 'Class' field");
		return false;
	}

	Out.Data = Node;
	return true;
}

bool ParseObject(const nlohmann::json& Node, LevelData_s::ObjectData_s& Out)
{
	if (!Node.is_object())
	{
		LOGWARNING("[Level] Object entry is not an object");
		return false;
	}

	if (!TryParseString(Node, "Class", Out.Class) || Out.Class.empty())
	{
		LOGWARNING("[Level] Object entry requires a 'Class' field");
		return false;
	}

	TryParseFloat3(Node, "Position", Out.Position);

	auto ComponentsIt = Node.find("Components");
	if (ComponentsIt != Node.end())
	{
		if (ComponentsIt->is_array())
		{
			Out.Components.reserve(ComponentsIt->size());

			for (const nlohmann::json& ComponentNode : *ComponentsIt)
			{
				LevelData_s::ComponentData_s ComponentData;
				if (ParseComponent(ComponentNode, ComponentData))
				{
					Out.Components.push_back(std::move(ComponentData));
				}
			}
		}
		else
		{
			LOGWARNING("[Level] Object '%S' has a 'Components' field that is not an array", Out.Class.c_str());
		}
	}

	Out.Data = Node;
	return true;
}

bool ParseLevel(const nlohmann::json& Root, LevelData_s& Out)
{
	if (!Root.is_object())
	{
		LOGERROR("[Level] Root node is not an object");
		return false;
	}

	auto VersionIt = Root.find("Version");
	if (VersionIt == Root.end() || !VersionIt->is_number_integer())
	{
		LOGERROR("[Level] Requires an integer 'Version' field");
		return false;
	}

	Out.Version = VersionIt->get<int32_t>();

	auto ObjectsIt = Root.find("Objects");
	if (ObjectsIt == Root.end())
	{
		LOGERROR("[Level] Requires an 'Objects' field");
		return false;
	}

	if (!ObjectsIt->is_array())
	{
		LOGERROR("[Level] Field 'Objects' is not an array");
		return false;
	}

	Out.Objects.reserve(ObjectsIt->size());

	for (const nlohmann::json& ObjectNode : *ObjectsIt)
	{
		LevelData_s::ObjectData_s ObjectData;
		if (ParseObject(ObjectNode, ObjectData))
		{
			Out.Objects.push_back(std::move(ObjectData));
		}
	}

	return true;
}

}

void Level_c::Deserialize(const std::wstring& LevelPath)
{
	using json = nlohmann::json;

	// Note: narrow stream, nlohmann only provides an input adapter for std::istream
	std::ifstream AssetFile(LevelPath.c_str());

	if (!AssetFile.is_open())
	{
		LOGERROR("[Level] Failed to open file %S", LevelPath.c_str());
		return;
	}

	constexpr bool AllowExceptions = false;
	json Data = json::parse(AssetFile, nullptr, AllowExceptions);

	if (Data.is_discarded())
	{
		LOGERROR("[Level] Failed to parse json in file %S", LevelPath.c_str());
		return;
	}

	LevelData_s LevelData;
	if (!ParseLevel(Data, LevelData))
	{
		LOGERROR("[Level] Failed to deserialize level %S", LevelPath.c_str());
		return;
	}

	LOGINFO("[Level] Deserialized %S, version %d, %zu objects", LevelPath.c_str(), LevelData.Version, LevelData.Objects.size());

	// TODO: Spawn the objects described by LevelData
}

void Level_c::Unload()
{
	if (Space_c* Space = GetSpace())
	{
		for (const std::shared_ptr<class Object_c>& LevelObject : Objects)
		{
			Space->DestroyObject(LevelObject.get());
		}
	}

	Objects.clear();
}
