#pragma once

#include <Object/ObjectComponent.h>

class HovercraftObject_c;

// Owns the rules of a game mode. Only the first flow found in the Space is active.
class GameFlowComponent_c : public ObjectComponent_c
{
	OBJECTCOMPONENT_BODY(GameFlowComponent_c, ObjectComponent_c)

	// Begin ObjectComponent_c
	virtual void OnCreate() override;
	// End ObjectComponent_c

	virtual void OnHovercraftDied(HovercraftObject_c& Hovercraft) {}

	bool IsActiveFlow() const { return Active; }

private:
	bool Active = false;
};
