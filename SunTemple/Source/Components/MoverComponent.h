#pragma once

#include <Object/SpatialObjectComponent.h>

#include <SurfMath.h>

class MoverComponent_c : public SpatialObjectComponent_c
{
	OBJECTCOMPONENT_BODY(MoverComponent_c, SpatialObjectComponent_c)

public:
	// Begin ObjectComponent_c interface
	virtual void Deserialize(const struct JsonValue_s& Data) override;
	virtual void OnCreate() override;
	virtual void Update(float Delta) override;
	// End ObjectComponent_c interface

protected:

	float3 Direction = float3(0.0f, 1.0f, 0.0f);
	float Distance = 5.0f;
	float Speed = 1.0f;

	float3 StartPos = {};
	float CurrentPhase = 0.0f;
};