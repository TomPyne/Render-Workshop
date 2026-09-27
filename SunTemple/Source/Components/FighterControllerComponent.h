#pragma once

#include <Object/ControllerComponent.h>

class FighterControllerComponent_c : public ControllerComponent_c
{
	OBJECTCOMPONENT_BODY(FighterControllerComponent_c, ControllerComponent_c)

	virtual void OnCreate() override;
	virtual void Update(float Delta) override;

protected:
	class SpatialObject_c* SpatialOwner = nullptr;
	class MeshComponent_c* FighterMeshComp = nullptr;

	float Pitch = 0.0f;
	float PitchRate = 1.0f;
	float RollStartPitchDegrees = 75.0f;
	float RollEndPitchDegrees = 105.0f;
};