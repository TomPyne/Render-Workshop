#pragma once

#include "Object/Object.h"
#include "Object/SpatialObjectComponent.h"

#include "Utility/Transform.h"

#include <SurfMath.h>

class SpatialObject_c : public Object_c
{
public:
	SpatialObject_c(const ObjectArgs_s& Args);
	virtual ~SpatialObject_c() = default;

	// Begin Object_c interface
	virtual void Deserialize(const JsonValue_s& Data) override;
	// End Object_c interface

	const Transform_s& GetTransform() const { return Transform; }

	void SetPosition(const float3& NewPosition) { Transform.SetPosition(NewPosition); }
	void SetRotation(const float3& NewRotation) { Transform.SetRotation(NewRotation); }
	void SetScale(float NewScale) { Transform.SetScale(NewScale); }

	void Translate(const float3& Translation) { Transform.SetPosition(Transform.GetPosition() + Translation); }

protected:
	Transform_s Transform;
};