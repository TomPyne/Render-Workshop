#pragma once

#include <Object/ObjectComponent.h>

// Marks its owner as a spawn point. The flow reads the owner's position and yaw.
class LandingPadComponent_c : public ObjectComponent_c
{
	OBJECTCOMPONENT_BODY(LandingPadComponent_c, ObjectComponent_c)
};
