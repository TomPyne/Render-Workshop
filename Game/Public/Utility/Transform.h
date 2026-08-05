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

	void Set(float3 InPosition = float3(0.0f), float3 InRotation = float3(0.0f), float InScale = 1.0f) noexcept
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

	// Basis vectors are derived from Rotation rather than read out of Matrix,
	// since Matrix has Scale and Position baked into it.
	float3 GetForwardVector() const noexcept
	{
		return GetDirectionFromEuler(Rotation);
	}

	float3 GetRightVector() const noexcept
	{
		const float cp = cosf(Rotation.x);
		const float sp = sinf(Rotation.x);
		const float cy = cosf(Rotation.y);
		const float sy = sinf(Rotation.y);
		const float cr = cosf(Rotation.z);
		const float sr = sinf(Rotation.z);

		return float3{ cr * cy + sr * sp * sy, sr * cp, sr * sp * cy - cr * sy };
	}

	float3 GetUpVector() const noexcept
	{
		return Cross(GetForwardVector(), GetRightVector());
	}

private:
	float3 Position;
	float3 Rotation;
	float Scale;
	matrix Matrix;
};