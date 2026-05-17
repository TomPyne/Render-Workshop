#pragma once

#include <SurfMath.h>

struct TransformFragment_s
{
	TransformFragment_s()
		: Position(0)
		, Rotation(0)
		, Scale(0)
	{
		Transform = MakeMatrixIdentity();
	}

	const float3& GetPosition() const {	return Position; }
	const float3& GetRotation() const { return Rotation; }
	const float GetScale() const { return Scale; }
	const matrix& GetTransform() const { return Transform; }

	void SetPosition(const float3& InPosition)
	{
		Position = InPosition;
		RecomputeTransform();
	}

	void SetRotation(const float3& InRotation)
	{
		Rotation = InRotation;
		RecomputeTransform();
	}

	void SetScale(float InScale)
	{
		Scale = InScale;
		RecomputeTransform();
	}

	void RecomputeTransform()
	{
		Transform = MakeMatrixIdentity();
		Transform = Transform * MakeMatrixScaling(Scale, Scale, Scale);
		Transform = Transform * MakeMatrixRotationFromVector(Rotation);
		Transform = Transform * MakeMatrixTranslation(Position);
	}

	float3 Position = {};
	float3 Rotation = {};
	float Scale = 0.0f;
	matrix Transform;
};