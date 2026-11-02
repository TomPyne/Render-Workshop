#include "Object/ObjectComponent.h"

#include "Object/Object.h"
#include "Object/PropertyVisitor.h"
#include "Space/Space.h"

ObjectComponent_c::ObjectComponent_c(const ObjectComponentArgs_s& Args)
	: Owner(Args.Owner)
{}

void ObjectComponent_c::Deserialize(const JsonValue_s& Data)
{
	JsonReadVisitor_c Reader(Data);
	VisitProperties(Reader);
}

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
