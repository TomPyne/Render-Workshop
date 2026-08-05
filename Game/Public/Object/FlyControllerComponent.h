#pragma once

#include "Object/ObjectComponent.h"

#include <SurfMath.h>

class FlyControllerComponent_c : public ObjectComponent_c
{
public:
	using ObjectComponent_c::ObjectComponent_c;
	virtual ~FlyControllerComponent_c() = default;

	virtual void Update(float Delta) override;

protected:
	float ViewPitch = 0.0f;
	float ViewYaw = 0.0f;
	float3 LookDir = { 0.0f, 0.0f, 1.0f };
};