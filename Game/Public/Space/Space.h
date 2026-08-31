#pragma once

#include "Object/Object.h"

#include <SurfMath.h>

#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

class CameraComponent_c;
class Level_c;
class MaterialShader_c;

class Space_c : public std::enable_shared_from_this<Space_c>
{
public:

	std::vector<std::shared_ptr<Object_c>> Objects;
	std::vector<std::shared_ptr<Level_c>> Levels;

	void Update(float Delta);

	template<class ObjectType>
	std::shared_ptr<ObjectType> CreateObject()
	{
		static_assert(std::is_same_v<ObjectType, typename ObjectType::Self>, "Object class is missing an OBJECT_BODY declaration");

		std::shared_ptr<ObjectType> NewObject = std::make_shared<ObjectType>(ObjectArgs_s{this});
		Objects.push_back(NewObject);
		NewObject->OnConstruct();
		NewObject->OnCreate();
		return NewObject;
	}

	// TODO: Defer destruction until end of frame.
	void DestroyObject(Object_c* Object);

	// Factory functions ////////////////////////////////////////////////////////////////
	template<class ObjectType>
	void RegisterObjectClass()
	{
		static_assert(std::is_same_v<ObjectType, typename ObjectType::Self>, "Object class is missing an OBJECT_BODY declaration");

		ObjectFactoryCallbacks[std::wstring(ObjectType::StaticClassName())] = [](const ObjectArgs_s& Args) -> std::shared_ptr<Object_c>
		{
			return std::make_shared<ObjectType>(Args);
		};
	}

	template<class ComponentType>
	void RegisterComponentClass()
	{
		static_assert(std::is_same_v<ComponentType, typename ComponentType::Self>, "Component class is missing an OBJECTCOMPONENT_BODY declaration");

		ComponentFactoryCallbacks[std::wstring(ComponentType::StaticClassName())] = [](const ObjectComponentArgs_s& Args) -> std::shared_ptr<ObjectComponent_c>
		{
			return std::make_shared<ComponentType>(Args);
		};
	}
	
	std::shared_ptr<Object_c> CreateObjectByName(const std::wstring& ClassName, const JsonValue_s* const Data = nullptr);
	std::shared_ptr<ObjectComponent_c> CreateComponentByName(Object_c* Owner, const std::wstring& ClassName, const JsonValue_s* const Data = nullptr);

	// Level functions ////////////////////////////////////////////////////////////////
	template<class LevelType>
	Level_c* LoadLevel()
	{
		std::shared_ptr<LevelType> NewLevel = std::make_shared<LevelType>(shared_from_this());
		Levels.push_back(NewLevel);
		LoadLevelInternal(NewLevel.get(), L"");
		return NewLevel.get();
	}

	Level_c* LoadLevel(const struct Path_s& Path);

	void UnloadLevel(Level_c* InLevel);

	// Camera

	std::vector<std::weak_ptr<CameraComponent_c>> CameraStack;

	void PushCameraComponent(CameraComponent_c* Camera);
	void PopCameraComponent(CameraComponent_c* Camera);
	CameraComponent_c* GetCamera() const;

protected:

	void LoadLevelInternal(Level_c* InLevel, const std::wstring& LevelPath);

private:

	std::unordered_map<std::wstring, std::function<std::shared_ptr<Object_c>(const ObjectArgs_s&)>> ObjectFactoryCallbacks;
	std::unordered_map<std::wstring, std::function<std::shared_ptr<ObjectComponent_c>(const ObjectComponentArgs_s&)>> ComponentFactoryCallbacks;
};