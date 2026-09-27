#pragma once

#include "Object/Object.h"

#include "Utility/Transform.h"

#include <SurfMath.h>

class SpatialObject_c : public Object_c
{
	OBJECT_BODY(SpatialObject_c, Object_c)

	SpatialObject_c(const ObjectArgs_s& Args);

	// Begin Object_c interface
	virtual void Deserialize(const JsonValue_s& Data) override;
	// End Object_c interface

	const Transform_s& GetTransform() const { return Transform; }

	// Each setter calls OnTransformed on the spatial components, as their world matrix includes this transform
	void SetPosition(const float3& NewPosition);
	void SetRotation(const float3& NewRotation);
	void SetRotation(quat NewRotation);
	void SetScale(const float3& NewScale);
	void SetScale(float NewScale);

	void Translate(const float3& Translation);

	void Rotate(quat Delta);
	void RotateLocal(quat Delta);

protected:
	Transform_s Transform;

private:
	void NotifyComponentsTransformed();
};