#include "Object/Object.h"
#include "Space/Space.h"

#include <Shared/FileUtils/JsonHelpers.h>
#include <Shared/FileUtils/JsonValue.h>
#include <Shared/Logging/Logging.h>

Object_c::Object_c(const ObjectArgs_s& Args)
	: OwningSpace(Args.OwningSpace->shared_from_this())
{
}

void Object_c::Deserialize(const JsonValue_s& Data)
{
	const nlohmann::json& Node = Data.Json;
	auto ComponentsIt = Node.find("Components");
	if (ComponentsIt != Node.end())
	{
		if (ComponentsIt->is_array())
		{
			std::wstring Class;
			for (const nlohmann::json& ComponentNode : *ComponentsIt)
			{
				if (ENSUREMSG(JsonHelpers::ParseWString(Node, "Class", Class), "[Object] Deserialized component entry does not have a 'Class'"))
				{
					JsonValue_s ComponentData(ComponentNode);
					AddComponentByName(Class, &ComponentData);
				}
			}
		}
		else
		{
			LOGWARNING("[Object] Object has a 'Components' field that is not an array");
		}
	}
}

void Object_c::Update(float Delta)
{
	ForEachComponent([Delta](ObjectComponent_c* Component)
	{
		Component->Update(Delta);
		return true;
	});
}

void Object_c::AddComponentByName(const std::wstring& ClassName, const JsonValue_s* const Data)
{
	if (Space_c* Space = GetSpace())
	{
		Space->CreateComponentByName(this, ClassName, Data);
	}
}
