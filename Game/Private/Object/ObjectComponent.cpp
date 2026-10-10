#include "Object/ObjectComponent.h"

#include "Object/Object.h"
#include "Space/Space.h"

ObjectComponent_c::ObjectComponent_c(const ObjectComponentArgs_s& Args)
	: Owner(Args.Owner)
{}

Space_c* ObjectComponent_c::GetSpace() const
{
	Object_c* const OwnerPtr = GetOwner();
	return OwnerPtr ? OwnerPtr->GetSpace() : nullptr;
}

RenderScene_s* ObjectComponent_c::GetRenderScene() const
{
	if (Space_c* Space = GetSpace())
	{
		return &Space->RenderScene;
	}
	return nullptr;
}

void ObjectComponent_c::DirtyRenderScene() const
{
	if (RenderScene_s* RenderScene = GetRenderScene())
	{
		RenderScene->DirtyRenderScene();
	}
}
