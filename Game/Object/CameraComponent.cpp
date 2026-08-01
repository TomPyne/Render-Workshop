#include "CameraComponent.h"

#include "Game/Input/Input.h"
#include "Game/Space/Space.h"
#include "Game/Utility/Transform.h"

void CameraComponent_c::OnCreate()
{
	// Register camera component
	if (Space_c* Space = GetSpace())
	{
		Space->RegisterCameraComponent(this);
	}
}

void CameraComponent_c::PreDestroy()
{
	// Unregister
	if (Space_c* Space = GetSpace())
	{
		Space->UnregisterCameraComponent(this);
	}
}

matrix CameraComponent_c::CalculateViewMatrix() const
{
	float3 Position = GetTransform().GetPosition();
	float3 LookDir = GetTransform().GetForwardVector();
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
