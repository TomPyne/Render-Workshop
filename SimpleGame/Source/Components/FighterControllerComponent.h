#pragma once

#include <Object/ObjectComponent.h>

class FighterControllerComponent_c : public ObjectComponent_c
{
public:
	using ObjectComponent_c::ObjectComponent_c;
	virtual ~FighterControllerComponent_c() = default;

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