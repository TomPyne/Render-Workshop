#include "Level/Level.h"

#include "Space/Space.h"

#include <Shared/Logging/Logging.h>
#include <Shared/FileUtils/JsonHelpers.h>
#include <Shared/FileUtils/JsonValue.h>

#include <fstream>

#define LEVEL_VERSION_INITIAL 1
#define LEVEL_VERSION_CURRENT LEVEL_VERSION_INITIAL

struct LevelData_s
{
	struct ComponentData_s
	{
		std::wstring Class;

		// The whole component node, for the component to parse its own fields from
		Json_t Data;
	};

	struct ObjectData_s
	{
		std::wstring Class;
		float3 Position = float3(0.0f);
		std::vector<ComponentData_s> Components;

		// The whole object node, for the object to parse its own fields from
		Json_t Data;
	};

	int32_t Version = 0;
	std::vector<ObjectData_s> Objects;
};

namespace
{

bool ParseComponent(const Json_t& Node, LevelData_s::ComponentData_s& Out)
{
	if (!Node.is_object())
	{
		LOGWARNING("[Level] Component entry is not an object");
		return false;
	}

	if (!JsonHelpers::ParseWString(Node, "Class", Out.Class) || Out.Class.empty())
	{
		LOGWARNING("[Level] Component entry requires a 'Class' field");
		return false;
	}

	Out.Data = Node;
	return true;
}

bool ParseObject(const Json_t& Node, LevelData_s::ObjectData_s& Out)
{
	if (!Node.is_object())
	{
		LOGWARNING("[Level] Object entry is not an object");
		return false;
	}

	if (!JsonHelpers::ParseWString(Node, "Class", Out.Class) || Out.Class.empty())
	{
		LOGWARNING("[Level] Object entry requires a 'Class' field");
		return false;
	}

	JsonHelpers::ParseFloat3(Node, "Position", Out.Position);

	auto ComponentsIt = Node.find("Components");
	if (ComponentsIt != Node.end())
	{
		if (ComponentsIt->is_array())
		{
			Out.Components.reserve(ComponentsIt->size());

			for (const Json_t& ComponentNode : *ComponentsIt)
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

bool ParseLevel(const Json_t& Root, LevelData_s& Out)
{
	if (!Root.is_object())
	{
		LOGERROR("[Level] Root node is not an object");
		return false;
	}

	if (!JsonHelpers::ParseInt(Root, "Version", Out.Version))
	{
		LOGERROR("[Level] Requires an integer 'Version' field");
		return false;
	}

	if (Out.Version != LEVEL_VERSION_CURRENT)
	{
		LOGERROR("[Level] Unsupported version %d, expected %d", Out.Version, LEVEL_VERSION_CURRENT);
		return false;
	}

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

	for (const Json_t& ObjectNode : *ObjectsIt)
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
	Json_t Data;
	if (!LoadJsonFromFile(LevelPath, Data))
	{
		LOGERROR("[Level] Failed to load json from file %S", LevelPath.c_str());
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

	if (Space_c* Space = GetSpace())
	{
		for (LevelData_s::ObjectData_s& Object : LevelData.Objects)
		{
			JsonValue_s ObjectData(Object.Data);
			Space->CreateObjectByName(Object.Class, &ObjectData);
		}
	}
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
