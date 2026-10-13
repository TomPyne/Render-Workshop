#pragma once

#include <Object/SpatialObjectComponent.h>

#include <SurfMath.h>

class RotatorComponent_c : public SpatialObjectComponent_c
{
	OBJECTCOMPONENT_BODY(RotatorComponent_c, SpatialObjectComponent_c)

public:
	// Begin ObjectComponent_c interface
	virtual void Deserialize(const struct JsonValue_s& Data) override;
	virtual void OnCreate() override;
	virtual void Update(float Delta) override;
	// End ObjectComponent_c interface

protected:

	// Serialized
	float3 PivotOffset = float3(0.0f);
	float3 Axis = float3(0.0f, 1.0f, 0.0f);
	float DegreesPerSecond = 90.0f;

	// Runtime
	float3 StartPos = {};
	quat StartRot = quat::Identity();
	float CurrentAngle = 0.0f;
};
