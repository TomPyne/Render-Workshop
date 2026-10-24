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
	JsonHelpers::ParseString(Data, "Name", Name);

	const Json_t& Node = Data.Json;
	auto ComponentsIt = Node.find("Components");
	if (ComponentsIt != Node.end())
	{
		if (ComponentsIt->is_array())
		{
			std::wstring Class;
			for (const Json_t& ComponentNode : *ComponentsIt)
			{
				if (ENSUREMSG(JsonHelpers::ParseWString(ComponentNode, "Class", Class), "[Object] Deserialized component entry does not have a 'Class'"))
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

void Object_c::OnCreate()
{
	HasBeenCreated = true;

	// Components added during this loop are created as they are added, so only visit the existing ones.
	const size_t ComponentCount = Components.size();
	for (size_t Index = 0; Index < ComponentCount; ++Index)
	{
		Components[Index]->OnCreate();
	}
}

void Object_c::OnDestroy()
{
	for (std::shared_ptr<ObjectComponent_c>& Component : Components)
	{
		Component->PreDestroy();
	}
	Components.clear();
}

void Object_c::Update(float Delta)
{
	// A component may add components or destroy its owner, which clears the list, so the size is checked every pass.
	// The local copy keeps the running component alive if its owner is destroyed under it.
	for (size_t Index = 0; Index < Components.size() && !Destroyed; ++Index)
	{
		const std::shared_ptr<ObjectComponent_c> Component = Components[Index];
		Component->Update(Delta);
	}
}

void Object_c::AddComponentByName(const std::wstring& ClassName, const JsonValue_s* const Data)
{
	if (Space_c* Space = GetSpace())
	{
		if (HasBeenCreated)
		{
			Space->CreateComponentByName(this, ClassName, Data);
		}
		else
		{
			Space->ConstructComponentByName(this, ClassName, Data);
		}		
	}
}
