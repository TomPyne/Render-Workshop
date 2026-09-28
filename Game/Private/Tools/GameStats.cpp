#include "Tools/GameStats.h"

#include "Object/CameraComponent.h"

#include <RenderImGui/imgui/imgui.h>
#include <SurfMath.h>

namespace GameStats
{
struct
{
	bool HasCamera = false;
	float3 CamPos = {};
	float3 CamForward = {};

	uint32_t NumPrimitives = 0;
} G;

void UpdateCamera(CameraComponent_c* Camera)
{
	if (!Camera)
	{
		G.HasCamera = false;
		return;
	}

	G.HasCamera = true;
	G.CamPos = Camera->GetWorldPosition();
	G.CamForward = Camera->GetWorldForward();
}

void UpdatePrimCount(uint32_t PrimCount)
{
	G.NumPrimitives = PrimCount;
}

void DrawGameStatsWindow(bool* Open)
{
	ImGui::SetNextWindowBgAlpha(0.7f);
	if (ImGui::Begin("Game", Open, ImGuiWindowFlags_AlwaysAutoResize))
	{
		if (G.HasCamera)
		{
			ImGui::Text("Cam Pos: %.1f, %.1f, %.1f", G.CamPos.x, G.CamPos.y, G.CamPos.z);
			ImGui::Text("Cam Dir: %.1f, %.1f, %.1f", G.CamForward.x, G.CamForward.y, G.CamForward.z);
		}

		ImGui::Text("Prims Drawn: %d", G.NumPrimitives);
	}
	ImGui::End();
}
}