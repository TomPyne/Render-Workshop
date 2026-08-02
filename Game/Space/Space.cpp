#include "Space.h"
#include "Game/Object/CameraComponent.h"
#include "Game/Object/Object.h"
#include "Game/Level/Level.h"

#include <Shared/Logging/Logging.h>

void Space_c::Update(float Delta)
{
	for (auto& Object : Objects)
	{
		Object->Update(Delta);
	}
}

void Space_c::DestroyObject(Object_c* Object)
{
	if (Object)
	{
		std::erase(Objects, Object->shared_from_this());
	}
}

std::shared_ptr<Object_c> Space_c::CreateObjectByName(const std::wstring& ClassName, const JsonValue_s* const Data)
{
	auto It = ObjectFactoryCallbacks.find(ClassName);
	if (!ENSUREMSG(It != ObjectFactoryCallbacks.end(), "No object class registered for name '%S'", ClassName.c_str()))
	{
		return nullptr;
	}

	std::shared_ptr<Object_c> NewObject = It->second(ObjectArgs_s{ this });
	Objects.push_back(NewObject);

	if (Data)
	{
		NewObject->Deserialize(*Data);
	}

	NewObject->OnCreate();
	return NewObject;
}

std::shared_ptr<ObjectComponent_c> Space_c::CreateComponentByName(Object_c* Owner, const std::wstring& ClassName, const JsonValue_s* const Data)
{
	if (!Owner)
		return nullptr;

	auto It = ComponentFactoryCallbacks.find(ClassName);
	if (!ENSUREMSG(It != ComponentFactoryCallbacks.end(), "No component class registered for name '%S'", ClassName.c_str()))
	{
		return nullptr;
	}

	std::shared_ptr<ObjectComponent_c> NewComponent = It->second(ObjectComponentArgs_s{ Owner->shared_from_this() });
	Owner->Components.push_back(NewComponent);

	if (Data)
	{
		NewComponent->Deserialize(*Data);
	}

	NewComponent->OnCreate();
	return NewComponent;
}

void Space_c::LoadLevelInternal(Level_c* InLevel, const std::wstring& LevelPath)
{
	CHECK(InLevel);

	if (!LevelPath.empty())
	{
		InLevel->Deserialize(LevelPath);
	}

	InLevel->Load();
}

void Space_c::UnloadLevel(Level_c* InLevel)
{
	if (InLevel)
	{
		InLevel->Unload();
		std::erase(Levels, InLevel->shared_from_this());
	}	
}

void Space_c::RegisterCameraComponent(CameraComponent_c* Camera)
{
	if(Camera)
	{
		if (ENSUREMSG(PrimaryCamera.expired(), "Multiple camera components registered to space. Only one is supported."))
		{
			PrimaryCamera = std::dynamic_pointer_cast<CameraComponent_c>(Camera->shared_from_this());
		}
	}
}

void Space_c::UnregisterCameraComponent(CameraComponent_c* Camera)
{
	if (Camera)
	{
		if (ENSUREMSG(Camera == PrimaryCamera.lock().get(), "Unregistering a camera that has not been registered."))
		{
			PrimaryCamera.reset();
		}
	}
}
