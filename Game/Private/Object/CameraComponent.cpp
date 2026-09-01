#include "Object/CameraComponent.h"

#include "Core/GameApp.h"
#include "Space/Space.h"
#include "Utility/Transform.h"

void CameraComponent_c::OnCreate()
{
	Super::OnCreate();

	// Register camera component
	if (Space_c* Space = GetSpace())
	{
		Space->PushCameraComponent(this);
	}
}

void CameraComponent_c::PreDestroy()
{
	// Unregister
	if (Space_c* Space = GetSpace())
	{
		Space->PopCameraComponent(this);
	}

	Super::PreDestroy();
}

matrix CameraComponent_c::CalculateViewMatrix() const
{
	float3 Position = GetWorldPosition();
	float3 LookDir = GetWorldForward();
	float3 Target = Position + LookDir;
	return MakeMatrixLookAtLH(Position, Target, float3{ 0, 1, 0 });
}

matrix CameraComponent_c::CalculateProjectionMatrix(float AspectRatio) const
{
	return MakeMatrixPerspectiveFovLH(ConvertToRadians(Fov), AspectRatio, NearZ, FarZ);
}

matrix CameraComponent_c::CalculateProjectionMatrix(u32 ScreenWidth, u32 ScreenHeight) const
{
	const float AspectRatio = static_cast<float>(ScreenWidth) / static_cast<float>(ScreenHeight);
	return CalculateProjectionMatrix(AspectRatio);
}

matrix CameraComponent_c::CalculateProjectionMatrix() const
{
	const uint2 ScreenSize = GApp->GetScreenSize();
	return CalculateProjectionMatrix(ScreenSize.x, ScreenSize.y);
}

matrix CameraComponent_c::CalculateViewProjectionMatrix() const
{
	return CalculateViewMatrix() * CalculateProjectionMatrix();
}

float2 CameraComponent_c::ScreenPosToNDC(int2 ScreenPosition) const
{
	const uint2 ScreenSize = GApp->GetScreenSize();
	const float2 ScreenPos = (float2(ScreenPosition) + float2(0.5f)) / float2(ScreenSize);

	return float2(ScreenPos.x * 2.0f - 1.0f, 1.0f - ScreenPos.y * 2.0f);
}

void CameraComponent_c::CalculateRayForScreenPosition(int2 ScreenPosition, float3& OutStart, float3& OutEnd) const
{
	const matrix InverseViewProjection = InverseMatrix(CalculateViewProjectionMatrix());
	const float2 NDC = ScreenPosToNDC(ScreenPosition);
	const float4 Near = TransformF4(float4(NDC.x, NDC.y, 0.0f, 1.0f), InverseViewProjection);
	const float4 Far = TransformF4(float4(NDC.x, NDC.y, 1.0f, 1.0f), InverseViewProjection);

	OutStart = Near.xyz / Near.w;
	OutEnd = Far.xyz / Far.w;
}
