#pragma once

#include "Object/SpatialObject.h"
#include "Space/Space.h"

#include <memory>
#include <vector>

class Level_c : public std::enable_shared_from_this<Level_c>
{
public:

	Level_c(const std::shared_ptr<Space_c>& InOwningSpace)
		: OwningSpace(InOwningSpace)
	{}

	virtual ~Level_c() = default;

	virtual void Deserialize(const std::wstring& LevelPath);
	virtual void Load() {}
	virtual void Unload();

	template<class ObjectType>
	std::shared_ptr<ObjectType> AddObjectToLevel()
	{
		if (Space_c* Space = GetSpace())
		{
			std::shared_ptr<ObjectType> Object = Space->CreateObject<ObjectType>();
			Objects.push_back(Object);
			return Object;
		}

		return nullptr;
	}

	template<class ComponentType>
	std::shared_ptr<SpatialObject_c> AddSpatialComponentToLevel()
	{
		if (Space_c* Space = GetSpace())
		{
			std::shared_ptr<SpatialObject_c> Object = Space->CreateObject<SpatialObject_c>();
			Objects.push_back(Object);
			Object->AddComponent<ComponentType>();
			return Object;
		}

		return nullptr;
	}

	std::vector<std::shared_ptr<Object_c>> Objects;

	std::weak_ptr<Space_c> OwningSpace;

	Space_c* GetSpace() const
	{
		return OwningSpace.lock().get();
	}
};