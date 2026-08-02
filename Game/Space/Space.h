#pragma once

#include "Game/Object/Object.h"

#include <SurfMath.h>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class CameraComponent_c;
class Level_c;

class Space_c : public std::enable_shared_from_this<Space_c>
{
public:

	std::vector<std::shared_ptr<Object_c>> Objects;
	std::vector<std::shared_ptr<Level_c>> Levels;

	void Update(float Delta);

	template<class ObjectType>
	std::shared_ptr<ObjectType> CreateObject()
	{
		std::shared_ptr<ObjectType> NewObject = std::make_shared<ObjectType>(ObjectArgs_s{this});
		Objects.push_back(NewObject);
		NewObject->OnCreate();
		return NewObject;
	}

	// TODO: Defer destruction until end of frame.
	void DestroyObject(Object_c* Object);

	// Factory functions ////////////////////////////////////////////////////////////////
	template<class ObjectType>
	void RegisterObjectClass(const std::wstring& ClassName)
	{
		ObjectFactoryCallbacks[ClassName] = [](const ObjectArgs_s& Args) -> std::shared_ptr<Object_c>
		{
			return std::make_shared<ObjectType>(Args);
		};
	}

	template<class ComponentType>
	void RegisterComponentClass(const std::wstring& ClassName)
	{
		ComponentFactoryCallbacks[ClassName] = [](const ObjectComponentArgs_s& Args) -> std::shared_ptr<ObjectComponent_c>
		{
			return std::make_shared<ComponentType>(Args);
		};
	}

	std::shared_ptr<Object_c> CreateObjectByName(const std::wstring& ClassName, const JsonValue_s* const Data = nullptr);
	std::shared_ptr<ObjectComponent_c> CreateComponentByName(Object_c* Owner, const std::wstring& ClassName, const JsonValue_s* const Data = nullptr);

	// Level functions ////////////////////////////////////////////////////////////////
	template<class LevelType>
	Level_c* LoadLevel(const std::wstring& LevelPath = L"")
	{
		std::shared_ptr<LevelType> NewLevel = std::make_shared<LevelType>(shared_from_this());
		Levels.push_back(NewLevel);
		LoadLevelInternal(NewLevel.get(), LevelPath);
		return NewLevel.get();
	}

	void UnloadLevel(Level_c* InLevel);

	// Camera
	
	std::weak_ptr<CameraComponent_c> PrimaryCamera;

	void RegisterCameraComponent(CameraComponent_c* Camera);
	void UnregisterCameraComponent(CameraComponent_c* Camera);

protected:

	void LoadLevelInternal(Level_c* InLevel, const std::wstring& LevelPath);

private:

	std::unordered_map<std::wstring, std::function<std::shared_ptr<Object_c>(const ObjectArgs_s&)>> ObjectFactoryCallbacks;
	std::unordered_map<std::wstring, std::function<std::shared_ptr<ObjectComponent_c>(const ObjectComponentArgs_s&)>> ComponentFactoryCallbacks;
};