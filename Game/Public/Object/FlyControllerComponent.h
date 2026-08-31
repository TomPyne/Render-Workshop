#pragma once

#include "Object/ControllerComponent.h"

#include <SurfMath.h>

class FlyControllerComponent_c : public ControllerComponent_c
{
	OBJECTCOMPONENT_BODY(FlyControllerComponent_c, ControllerComponent_c)

	virtual void Update(float Delta) override;
};
