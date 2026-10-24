#pragma once

#include "AI/NavGrid.h"
#include "Components/GameFlowComponent.h"

// Runs the AI gym: spawns enemies from nests and sends them to death volumes
class AIGymFlowComponent_c : public GameFlowComponent_c
{
	OBJECTCOMPONENT_BODY(AIGymFlowComponent_c, GameFlowComponent_c)

	// Begin ObjectComponent_c
	virtual void OnCreate() override;
	// End ObjectComponent_c

protected:
	NavGrid_c NavGrid;
};
