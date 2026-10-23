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
	virtual void OnDestroy() override;
	// End Object_c interface

	// Local to the parent, or world when there is no parent
	const Transform_s& GetTransform() const { return Transform; }

	matrix GetWorldMatrix() const;
	float3 GetWorldPosition() const { return GetWorldMatrix().r[3].xyz; }
	quat GetWorldRotation() const;

	// The local transform is kept, so the object follows its new parent rather than holding its world placement
	void SetParent(SpatialObject_c* NewParent);
	SpatialObject_c* GetParent() const { return Parent.lock().get(); }

	// Each setter calls OnTransformed on the spatial components, as their world matrix includes this transform
	void SetPosition(const float3& NewPosition);
	void SetRotation(const float3& NewRotation);
	void SetRotation(quat NewRotation);
	void SetScale(const float3& NewScale);
	void SetScale(float NewScale);

	void Translate(const float3& Translation);

	void Rotate(quat Delta);
	void RotateLocal(quat Delta);

	// Call after a teleport, resets motion on every spatial component as their world matrix includes this transform
	void ResetMotion();

protected:
	Transform_s Transform;

private:
	// Recurses into children, as their world matrix includes this transform
	void NotifyComponentsTransformed();

	std::weak_ptr<SpatialObject_c> Parent;
	std::vector<std::weak_ptr<SpatialObject_c>> Children;
};