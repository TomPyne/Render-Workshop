#pragma once

#include "Object/Object.h"
#include "Object/SpatialObjectComponent.h"

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

	void SetPosition(const float3& NewPosition) { Transform.SetPosition(NewPosition); }
	void SetRotation(const float3& NewRotation) { Transform.SetRotation(NewRotation); }
	void SetRotation(quat NewRotation) { Transform.SetRotation(NewRotation); }
	void SetScale(float NewScale) { Transform.SetScale(NewScale); }

	void Translate(const float3& Translation) { Transform.SetPosition(Transform.GetPosition() + Translation); }

	void Rotate(quat Delta) { Transform.Rotate(Delta); }
	void RotateLocal(quat Delta) { Transform.RotateLocal(Delta); }

protected:
	Transform_s Transform;
};