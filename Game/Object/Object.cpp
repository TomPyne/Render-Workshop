#include "Object.h"

void Object_c::Update(float Delta)
{
	ForEachComponent([Delta](ObjectComponent_c* Component)
	{
		Component->Update(Delta);
		return true;
	});
}
