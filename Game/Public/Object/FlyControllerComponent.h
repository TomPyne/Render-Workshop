#pragma once

#include "Object/ObjectComponent.h"

#include <SurfMath.h>

class FlyControllerComponent_c : public ObjectComponent_c
{
	OBJECTCOMPONENT_BODY(FlyControllerComponent_c, ObjectComponent_c)

	virtual void Update(float Delta) override;

protected:
	float ViewPitch = 0.0f;
	float ViewYaw = 0.0f;
};