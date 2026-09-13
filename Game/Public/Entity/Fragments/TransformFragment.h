#pragma once

#include "Utility/Transform.h"

#include <SurfMath.h>

struct TransformFragment_s
{
	float3 GetPosition() const { return Transform.GetPosition(); }
	float3 GetRotation() const { return Transform.GetRotation(); }
	quat GetRotationQuat() const { return Transform.GetRotationQuat(); }
	float3 GetScale() const { return Transform.GetScale(); }
	const matrix& GetTransform() const { return Transform.GetMatrix(); }

	void SetPosition(const float3& InPosition) { Transform.SetPosition(InPosition); }
	void SetRotation(const float3& InRotation) { Transform.SetRotation(InRotation); }
	void SetRotation(quat InRotation) { Transform.SetRotation(InRotation); }
	void SetScale(const float3& InScale) { Transform.SetScale(InScale); }
	void SetScale(float InScale) { Transform.SetScale(InScale); }

	void Rotate(quat Delta) { Transform.Rotate(Delta); }
	void RotateLocal(quat Delta) { Transform.RotateLocal(Delta); }

	Transform_s Transform;
};
