#include "Object/CameraComponent.h"

#include "Input/Input.h"
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
