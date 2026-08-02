#pragma once

#include "ObjectComponent.h"

#include <SurfMath.h>

class SpatialObject_c;

// TODO - add child offsets
class SpatialObjectComponent_c : public ObjectComponent_c
{
public:
	SpatialObjectComponent_c(const ObjectComponentArgs_s& Args);
	virtual ~SpatialObjectComponent_c() = default;

	const struct Transform_s& GetTransform() const;

	SpatialObject_c* GetSpatialOwner() const
	{
		return SpatialOwner.lock().get();
	}

private:

	std::weak_ptr<SpatialObject_c> SpatialOwner;
};