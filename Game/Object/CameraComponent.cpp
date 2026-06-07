#include "CameraComponent.h"

#include "Game/Input/Input.h"
#include "Game/Space/Space.h"

void CameraComponent_c::OnCreate()
{
	// Register camera component
	if (Space_c* Space = GetSpace())
	{
		Space->RegisterCameraComponent(this);
	}
}

void CameraComponent_c::PreDestroy()
{
	// Unregister
	if (Space_c* Space = GetSpace())
	{
		Space->UnregisterCameraComponent(this);
	}
}
