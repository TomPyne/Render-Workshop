#include "FlyCamera.h"
#include <imgui.h>

void FlyCamera::SetView(const float3& InPosition, float Pitch, float Yaw)
{
	Position = InPosition;

	if (Yaw > 360.0f)
		Yaw -= 360.0f;

	if (Yaw < -360.0f)
		Yaw += 360.0f;

	CamPitch = Pitch;
	CamYaw = Yaw;

	Pitch = Clamp(Pitch, -85.0f, 85.0f);

	Yaw = ConvertToRadians(Yaw);
	Pitch = ConvertToRadians(Pitch);

	float CosPitch = cosf(Pitch);

	LookDir = float3{ cosf(Yaw) * CosPitch, sinf(Pitch), sinf(Yaw) * CosPitch };

	View = MakeMatrixLookToLH(InPosition, LookDir, float3{ 0, 1, 0 });
}

void FlyCamera::UpdateView(float delta)
{
	ImGuiIO& io = ImGui::GetIO();

	//float CamPitch = CamPitch;
	//float CamYaw = CamYaw;

	if (!io.WantCaptureMouse && io.MouseDown[1])
	{
		float Yaw = ImGui::GetIO().MouseDelta.x;
		float Pitch = ImGui::GetIO().MouseDelta.y;

		CamPitch -= Pitch * 25.0f * delta;
		CamYaw -= Yaw * 25.0f * delta;
	}

	float3 translation = { 0.0f };

	if (!io.WantCaptureKeyboard)
	{
		float3 Fwd = LookDir;
		float3 Rgt = Cross(float3{ 0, 1, 0 }, LookDir);

		constexpr float Speed = 5.0f;

		float MoveSpeed = Speed * delta;

		float3 TranslateDir = 0.0f;

		if (io.KeysDown[ImGuiKey_W]) TranslateDir += Fwd;
		if (io.KeysDown[ImGuiKey_S]) TranslateDir -= Fwd;

		if (io.KeysDown[ImGuiKey_D]) TranslateDir += Rgt;
		if (io.KeysDown[ImGuiKey_A]) TranslateDir -= Rgt;

		if (io.KeyShift)
			MoveSpeed *= 4.0f;

		translation = Normalize(TranslateDir) * MoveSpeed;

		if (io.KeysDown[ImGuiKey_E]) translation.y += MoveSpeed;
		if (io.KeysDown[ImGuiKey_Q]) translation.y -= MoveSpeed;
	}

	SetView(Position + translation, CamPitch, CamYaw);
}

Frustum FlyCamera::GetWorldFrustum() const
{
	return MakeWorldFrustum(Position, LookDir, float3(0, 1, 0), ConvertToRadians(Fov), AspectRatio, NearZ, FarZ);
}
