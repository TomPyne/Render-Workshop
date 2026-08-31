#include "Object/ControllerComponent.h"

#include "Space/Space.h"

void ControllerComponent_c::OnCreate()
{
	Super::OnCreate();

	if (Space_c* Space = GetSpace())
	{
		Space->PushControllerComponent(this);
	}
}

void ControllerComponent_c::PreDestroy()
{
	if (Space_c* Space = GetSpace())
	{
		Space->PopControllerComponent(this);
	}

	Super::PreDestroy();
}

bool ControllerComponent_c::IsActiveController() const
{
	const Space_c* Space = GetSpace();
	return Space && Space->IsControllerActive(this);
}
