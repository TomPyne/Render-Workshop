#pragma once

#include "Components/GameFlowComponent.h"

#include <SurfMath.h>

#include <memory>

// Spawns the hovercraft on a landing pad and respawns it there after it dies.
class HovercraftGymFlowComponent_c : public GameFlowComponent_c
{
	OBJECTCOMPONENT_BODY(HovercraftGymFlowComponent_c, GameFlowComponent_c)

	// Begin ObjectComponent_c
	virtual void OnCreate() override;
	virtual void Update(float Delta) override;
	// End ObjectComponent_c

	// Begin GameFlowComponent_c
	virtual void OnHovercraftDied(HovercraftObject_c& Hovercraft) override;
	// End GameFlowComponent_c

	float RespawnDelay = 3.0f;

protected:
	// Placeholder rules: the first pad found wins, and no pads is an ensure.
	bool FindSpawnPad(float3& OutPosition, float& OutYaw) const;

	std::weak_ptr<HovercraftObject_c> Hovercraft;

	float RespawnTimeRemaining = 0.0f;
	bool RespawnPending = false;
};
