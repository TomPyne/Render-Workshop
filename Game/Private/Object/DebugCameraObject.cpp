#include "Object/DebugCameraObject.h"

#include "Object/CameraComponent.h"
#include "Object/FlyControllerComponent.h"

void DebugCameraObject_c::OnConstruct()
{
	Super::OnConstruct();

	AddComponent<CameraComponent_c>();
	AddComponent<FlyControllerComponent_c>();
}
