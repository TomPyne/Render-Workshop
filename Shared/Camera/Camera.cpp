#include "Camera.h"

void Camera::Resize(u32 InScreenWidth, u32 InScreenHeight)
{
	if (InScreenWidth != ScreenWidth || InScreenHeight != ScreenHeight)
	{
		ScreenWidth = InScreenWidth;
		ScreenHeight = InScreenHeight;
		AspectRatio = (float)InScreenWidth / (float)InScreenHeight;
		RecalculateMatrices();
	}
}

void Camera::SetNearFar(float InNearZ, float InFarZ)
{
	if (InNearZ != NearZ || InFarZ != FarZ)
	{
		NearZ = InNearZ;
		FarZ = InFarZ;
		RecalculateMatrices();
	}
}

void Camera::SetFov(float InFov)
{
	if (Fov != InFov)
	{
		Fov = InFov;
		RecalculateMatrices();
	}
}

Frustum Camera::CalculateViewFrustum() const noexcept
{
	return MakeFrustum(ConvertToRadians(Fov), AspectRatio, NearZ, FarZ);
}

void Camera::RecalculateMatrices()
{
	Projection = MakeMatrixPerspectiveFovLH(ConvertToRadians(Fov), AspectRatio, NearZ, FarZ);

	const float YScale = 1.0f / tanf(ConvertToRadians(Fov) * 0.5f);
	const float XScale = YScale / AspectRatio;
	const float X2 = (float)ScreenWidth * 0.5f;
	const float Y2 = (float)ScreenHeight * 0.5f;
	const float X = XScale * X2;
	const float Y = -YScale * Y2;
	const float ZMin = FarZ / (FarZ - NearZ);
	const float ZMax = -NearZ * FarZ / (FarZ - NearZ);

	PixelProjection = matrix(
		X, 0, 0,    X2,
		0, Y, 0,    Y2,
		0, 0, ZMin, ZMax,
		0, 0, 1,    0);
}
