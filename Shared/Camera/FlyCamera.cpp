#include "FlyCamera.h"
#include <imgui.h>

void FlyCamera::SetView(const float3& InPosition, float Pitch, float Yaw)
{
	Position = InPosition;

	// Store the clamped and wrapped angles, so pitch cannot run away past the clamp
	// and leave the view unresponsive until it has been wound all the way back.
	CamPitch = Clamp(Pitch, -85.0f, 85.0f);
	CamYaw = fmodf(Yaw, 360.0f);

	// Canonical convention, shared with GetDirectionFromEuler and the fly controller:
	// yaw zero faces +Z and positive pitch tilts down.
	const quat Rotation = QuatFromEuler(float3{ ConvertToRadians(CamPitch), ConvertToRadians(CamYaw), 0.0f });

	LookDir = Rotate(Rotation, float3{ 0.0f, 0.0f, 1.0f });

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

		// Both signs follow the canonical convention above, so the feel is unchanged:
		// dragging down looks down, dragging right turns right.
		CamPitch += Pitch * 25.0f * delta;
		CamYaw += Yaw * 25.0f * delta;
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
