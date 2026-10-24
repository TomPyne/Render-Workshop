#pragma once

#include <Object/ObjectComponent.h>

#include <SurfMath.h>

// Marks its owner as a death volume. Enemies die when they touch its axis-aligned box in XZ.
class DeathVolumeComponent_c : public ObjectComponent_c
{
	OBJECTCOMPONENT_BODY(DeathVolumeComponent_c, ObjectComponent_c)

	// From the owner's scale, as the cube mesh spans -1 to 1. Rotation is ignored.
	float2 GetFootprintHalfExtents() const;
};
