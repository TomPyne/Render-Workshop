#pragma once

#include "ObjectComponent.h"

#include <SurfMath.h>

class SpatialObject_c;

// TODO - add child offsets
class SpatialObjectComponent_c : public ObjectComponent_c
{
public:
	SpatialObjectComponent_c(const std::shared_ptr<SpatialObject_c>& InSpatialOwner);
	virtual ~SpatialObjectComponent_c() = default;

	float3 GetPosition() const;
	float3 GetRotation() const;
	float GetScale() const;
	matrix GetTransform() const;

	SpatialObject_c* GetSpatialOwner() const
	{
		return SpatialOwner.lock().get();
	}

private:

	std::weak_ptr<SpatialObject_c> SpatialOwner;
};