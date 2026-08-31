#include "Space/Space.h"
#include "Object/CameraComponent.h"
#include "Object/Object.h"
#include "Level/Level.h"
#include "Utility/SharedPtr.h"

#include <Shared/FileUtils/PathUtils.h>
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
		Object->OnDestroy();
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

	NewObject->OnConstruct();

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

	NewComponent->OnConstruct();

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

Level_c* Space_c::LoadLevel(const Path_s& Path)
{
	std::shared_ptr<Level_c> NewLevel = std::make_shared<Level_c>(shared_from_this());
	Levels.push_back(NewLevel);
	LoadLevelInternal(NewLevel.get(), Path.ToWString());
	return NewLevel.get();
}

void Space_c::UnloadLevel(Level_c* InLevel)
{
	if (InLevel)
	{
		InLevel->Unload();
		std::erase(Levels, InLevel->shared_from_this());
	}	
}

void Space_c::PushCameraComponent(CameraComponent_c* Camera)
{
	if(Camera)
	{
		CameraStack.push_back(SharedFrom(Camera));
	}
}

void Space_c::PopCameraComponent(CameraComponent_c* Camera)
{
	if (Camera)
	{
		std::erase_if(CameraStack, [Camera](const std::weak_ptr<CameraComponent_c>& Registered)
		{
			const std::shared_ptr<CameraComponent_c> RegisteredShard = Registered.lock();
			return RegisteredShard && RegisteredShard.get() == Camera;			
		});
	}
}

CameraComponent_c* Space_c::GetCamera() const
{
	if (CameraStack.empty())
		return nullptr;

	return CameraStack.back().lock().get();
}
