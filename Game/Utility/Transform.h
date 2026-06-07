#pragma once

#include <SurfMath.h>

struct Transform_s
{
	Transform_s()
	{
		Set();
	}

	Transform_s(float3 InPosition)
	{
		Set(InPosition);
	}

	Transform_s(float3 InPosition, float3 InRotation)
	{
		Set(InPosition, InRotation);
	}

	Transform_s(float3 InPosition, float3 InRotation, float InScale)
	{
		Set(InPosition, InRotation, InScale);
	}

	void Set(float3 InPosition = float3(0.0f), float3 InRotation = float3(0.0f), float InScale = 0.0f) noexcept
	{
		Position = InPosition;
		Rotation = InRotation;
		Scale = InScale;
		UpdateMatrix();
	}

	void SetPosition(float3 InPosition) noexcept
	{
		Position = InPosition;
		UpdateMatrix();
	}

	void SetRotation(float3 InRotation) noexcept
	{
		Rotation = InRotation;
		UpdateMatrix();
	}

	void SetScale(float InScale) noexcept
	{
		Scale = InScale;
		UpdateMatrix();
	}

	void UpdateMatrix() noexcept
	{
		Matrix = MakeMatrixScaling(Scale, Scale, Scale);
		Matrix = Matrix * MakeMatrixRotationFromVector(Rotation);
		Matrix = Matrix * MakeMatrixTranslation(Position);
	}

	float3 GetPosition() const noexcept { return Position; }
	float3 GetRotation() const noexcept { return Rotation; }
	float GetScale() const noexcept { return Scale; }
	const matrix& GetMatrix() const noexcept { return Matrix; }

private:
	float3 Position;
	float3 Rotation;
	float Scale;
	matrix Matrix;
};