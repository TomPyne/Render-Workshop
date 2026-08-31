#pragma once

#include "Object/ObjectComponent.h"

class ControllerComponent_c : public ObjectComponent_c
{
	OBJECTCOMPONENT_BODY(ControllerComponent_c, ObjectComponent_c)

	// Begin ObjectComponent_c
	virtual void OnCreate() override;
	virtual void PreDestroy() override;
	// End ObjectComponent_c

	bool IsActiveController() const;
};