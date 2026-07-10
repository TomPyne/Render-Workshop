#include "Object.h"
#include "Game/Space/Space.h"

Object_c::Object_c(const ObjectArgs_s& Args)
	: OwningSpace(Args.OwningSpace->shared_from_this())
{
}

void Object_c::Update(float Delta)
{
	ForEachComponent([Delta](ObjectComponent_c* Component)
	{
		Component->Update(Delta);
		return true;
	});
}
