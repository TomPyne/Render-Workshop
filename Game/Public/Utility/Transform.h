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

	Transform_s(float3 InPosition, quat InRotation, float InScale = 1.0f)
	{
		Set(InPosition, InRotation, InScale);
	}

	void Set(float3 InPosition = float3(0.0f), float3 InRotation = float3(0.0f), float InScale = 1.0f) noexcept
	{
		Set(InPosition, QuatFromEuler(InRotation), InScale);
	}

	void Set(float3 InPosition, quat InRotation, float InScale) noexcept
	{
		Position = InPosition;
		Rotation = Normalize(InRotation);
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
		SetRotation(QuatFromEuler(InRotation));
	}

	void SetRotation(quat InRotation) noexcept
	{
		Rotation = Normalize(InRotation);
		UpdateMatrix();
	}

	void SetScale(float InScale) noexcept
	{
		Scale = InScale;
		UpdateMatrix();
	}

	// Delta is applied in parent space, RotateLocal applies it about our own axes.
	void Rotate(quat Delta) noexcept
	{
		SetRotation(Mul(Rotation, Delta));
	}

	void RotateLocal(quat Delta) noexcept
	{
		SetRotation(Mul(Delta, Rotation));
	}

	void UpdateMatrix() noexcept
	{
		Matrix = MakeMatrixScaling(Scale, Scale, Scale);
		Matrix = Matrix * MakeMatrixRotationFromQuaternion(Rotation);
		Matrix = Matrix * MakeMatrixTranslation(Position);
	}

	float3 GetPosition() const noexcept { return Position; }
	quat GetRotationQuat() const noexcept { return Rotation; }
	float3 GetRotation() const noexcept { return QuatToEuler(Rotation); }
	float GetScale() const noexcept { return Scale; }
	const matrix& GetMatrix() const noexcept { return Matrix; }

	// Qualified, since the Rotate member above hides the SurfMath free function.
	float3 GetForwardVector() const noexcept
	{
		return ::Rotate(Rotation, float3{ 0.0f, 0.0f, 1.0f });
	}

	float3 GetRightVector() const noexcept
	{
		return ::Rotate(Rotation, float3{ 1.0f, 0.0f, 0.0f });
	}

	float3 GetUpVector() const noexcept
	{
		return ::Rotate(Rotation, float3{ 0.0f, 1.0f, 0.0f });
	}

private:
	float3 Position;
	quat Rotation;
	float Scale;
	matrix Matrix;
};
