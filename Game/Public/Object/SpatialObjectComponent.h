#pragma once

#include "Object/ObjectComponent.h"

#include "Utility/Transform.h"

#include <SurfMath.h>

class SpatialObject_c;

class SpatialObjectComponent_c : public ObjectComponent_c
{
	OBJECTCOMPONENT_BODY(SpatialObjectComponent_c, ObjectComponent_c)

	SpatialObjectComponent_c(const ObjectComponentArgs_s& Args);

	// Begin ObjectComponent_c interface
	virtual void Deserialize(const struct JsonValue_s& Data) override;
	// End ObjectComponent_c interface

	const Transform_s& GetTransform() const { return Transform; }

	void SetPosition(const float3& NewPosition) { Transform.SetPosition(NewPosition); }
	void SetRotation(const float3& NewRotation) { Transform.SetRotation(NewRotation); }
	void SetScale(float NewScale) { Transform.SetScale(NewScale); }

	const matrix& GetWorldMatrix() const;

	float3 GetWorldPosition() const { return GetWorldMatrix().r[3].xyz; }
	float3 GetWorldForward() const { return Normalize(GetWorldMatrix().r[2].xyz); }
	float3 GetWorldRotation() const;

	SpatialObject_c* GetSpatialOwner() const
	{
		return SpatialOwner.lock().get();
	}

protected:
	Transform_s Transform;

private:

	std::weak_ptr<SpatialObject_c> SpatialOwner;

	mutable matrix WorldMatrix;
};