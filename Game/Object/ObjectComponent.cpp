#include "ObjectComponent.h"

#include "Object.h"

ObjectComponent_c::ObjectComponent_c(const ObjectComponentArgs_s& Args)
	: Owner(Args.Owner)
{}

Space_c* ObjectComponent_c::GetSpace() const
{
	Object_c* const OwnerPtr = GetOwner();
	return OwnerPtr ? OwnerPtr->GetSpace() : nullptr;
}
