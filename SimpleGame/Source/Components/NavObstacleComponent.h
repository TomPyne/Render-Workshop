#pragma once

#include <Object/ObjectComponent.h>

// Marks its owner as a nav grid obstacle. The footprint is the owner's oriented box in XZ.
class NavObstacleComponent_c : public ObjectComponent_c
{
	OBJECTCOMPONENT_BODY(NavObstacleComponent_c, ObjectComponent_c)
};
