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

void Space_c::LoadLevelInternal(Level_c* InLevel)
{
	CHECK(InLevel);
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
