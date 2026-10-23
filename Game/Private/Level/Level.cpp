#include "Level/Level.h"

#include "Space/Space.h"

#include <Shared/Logging/Logging.h>
#include <Shared/FileUtils/JsonHelpers.h>
#include <Shared/FileUtils/JsonValue.h>

#include <algorithm>
#include <fstream>
#include <unordered_map>

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

struct LoadContext_s
{
	// Levels currently being instantiated, outermost first, to catch a level that includes itself
	std::vector<std::wstring> PathStack;

	// Every object constructed by the load, nested levels included, so they are all created once the whole tree exists
	std::vector<std::shared_ptr<Object_c>> PendingCreate;
};

LoadContext_s* ActiveLoad = nullptr;

// Parsed once per path, so a level instanced many times only reads its file once
std::shared_ptr<const LevelData_s> GetLevelData(const std::wstring& LevelPath)
{
	static std::unordered_map<std::wstring, std::shared_ptr<const LevelData_s>> Cache;

	auto It = Cache.find(LevelPath);
	if (It != Cache.end())
	{
		return It->second;
	}

	Json_t Data;
	if (!LoadJsonFromFile(LevelPath, Data))
	{
		LOGERROR("[Level] Failed to load json from file %S", LevelPath.c_str());
		return nullptr;
	}

	std::shared_ptr<LevelData_s> LevelData = std::make_shared<LevelData_s>();
	if (!ParseLevel(Data, *LevelData))
	{
		LOGERROR("[Level] Failed to deserialize level %S", LevelPath.c_str());
		return nullptr;
	}

	LOGINFO("[Level] Deserialized %S, version %d, %zu objects", LevelPath.c_str(), LevelData->Version, LevelData->Objects.size());

	Cache.emplace(LevelPath, LevelData);
	return LevelData;
}

}

void Level_c::Deserialize(const std::wstring& LevelPath, SpatialObject_c* Parent)
{
	Space_c* Space = GetSpace();
	if (!Space)
		return;

	LoadContext_s OutermostLoad;
	const bool IsOutermost = !ActiveLoad;
	if (IsOutermost)
	{
		ActiveLoad = &OutermostLoad;
	}

	std::shared_ptr<const LevelData_s> LevelData;
	if (std::ranges::find(ActiveLoad->PathStack, LevelPath) != ActiveLoad->PathStack.end())
	{
		LOGERROR("[Level] %S includes itself, skipping the nested instance", LevelPath.c_str());
	}
	else
	{
		LevelData = GetLevelData(LevelPath);
	}

	if (LevelData)
	{
		ActiveLoad->PathStack.push_back(LevelPath);

		for (const LevelData_s::ObjectData_s& Object : LevelData->Objects)
		{
			// Constructing a level instance loads its contents into PendingCreate, so reserve this object's slot first to keep parents created before children.
			const size_t CreateSlot = ActiveLoad->PendingCreate.size();

			JsonValue_s ObjectData(Object.Data);
			std::shared_ptr<Object_c> NewObject = Space->ConstructObjectByName(Object.Class, &ObjectData);
			if (!NewObject)
				continue;

			if (Parent)
			{
				if (SpatialObject_c* SpatialObject = dynamic_cast<SpatialObject_c*>(NewObject.get()))
				{
					SpatialObject->SetParent(Parent);
				}
			}

			Objects.push_back(NewObject);
			ActiveLoad->PendingCreate.insert(ActiveLoad->PendingCreate.begin() + CreateSlot, NewObject);
		}

		ActiveLoad->PathStack.pop_back();
	}

	if (IsOutermost)
	{
		ActiveLoad = nullptr;

		// OnCreate may spawn objects into the Space, so walk the load's own list rather than the Space's.
		for (const std::shared_ptr<Object_c>& Object : OutermostLoad.PendingCreate)
		{
			Object->OnCreate();
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
