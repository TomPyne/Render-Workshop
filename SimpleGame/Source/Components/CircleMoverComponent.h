#pragma once

#include <Object/ObjectComponent.h>

#include <SurfMath.h>

// Moves its owner around a circle in XZ, centred on the owner's position at create.
class CircleMoverComponent_c : public ObjectComponent_c
{
	OBJECTCOMPONENT_BODY(CircleMoverComponent_c, ObjectComponent_c)

	// Begin ObjectComponent_c
	virtual void Deserialize(const struct JsonValue_s& Data) override;
	virtual void OnCreate() override;
	virtual void Update(float Delta) override;
	// End ObjectComponent_c

	float Radius = 20.0f;
	float Period = 20.0f;

protected:
	float3 Centre = float3(0.0f);
	float Angle = 0.0f;
};
