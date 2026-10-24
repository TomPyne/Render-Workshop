#pragma once

#include "Components/GameFlowComponent.h"

// Runs the AI gym: spawns enemies from nests and sends them to death volumes
class AIGymFlowComponent_c : public GameFlowComponent_c
{
	OBJECTCOMPONENT_BODY(AIGymFlowComponent_c, GameFlowComponent_c)
};
