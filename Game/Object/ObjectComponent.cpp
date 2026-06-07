#include "ObjectComponent.h"

#include "Object.h"

ObjectComponent_c::ObjectComponent_c(const std::shared_ptr<Object_c>& InOwner)
	: Owner(InOwner)
{}

Space_c* ObjectComponent_c::GetSpace() const
{
	Object_c* const OwnerPtr = GetOwner();
	return OwnerPtr ? OwnerPtr->GetSpace() : nullptr;
}
