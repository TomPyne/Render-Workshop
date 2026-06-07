#pragma once

#include "SpatialObjectComponent.h"

class CameraComponent_c : public SpatialObjectComponent_c
{
public:	
	virtual ~CameraComponent_c() = default;

	virtual void OnCreate() override;

	virtual void PreDestroy() override;

	float NearZ = 0.1f;
	float FarZ = 10'000.0f;
	float Fov = 45.0f;
};