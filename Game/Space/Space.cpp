#include "Space.h"
#include "Game/Object/CameraComponent.h"
#include "Game/Object/Object.h"

#include <Shared/Logging/Logging.h>

void Space_c::Update(float Delta)
{
	for (auto& Object : Objects)
	{
		Object->Update(Delta);
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
