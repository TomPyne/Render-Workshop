#include "Tools/LightingViewer.h"

#include "Rendering/SpaceRenderer.h"

#include <RenderImGui/imgui/imgui.h>

namespace LightingViewer
{
namespace
{

void DrawShadowTemporal(SpaceRenderer_c& Renderer)
{
	ShadowDenoiseSettings_s& Settings = Renderer.ShadowDenoise;

	ImGui::Checkbox("Enabled##Temporal", &Settings.TemporalEnabled);
	ImGui::SliderFloat("Max confidence", &Settings.MaxConfidence, 0.0f, 0.99f, "%.2f");
	ImGui::SliderFloat("Confidence rate", &Settings.ConfidenceRate, 0.01f, 1.0f, "%.2f");
	ImGui::SliderFloat("Depth tolerance", &Settings.DepthTolerance, 0.001f, 0.1f, "%.3f");

	if (ImGui::Button("Reset history"))
	{
		Renderer.GetShadowDenoiser().ResetHistory();
	}
}

}

void DrawWindow(bool* Open, SpaceRenderer_c* Renderer)
{
	if (!ImGui::Begin("Lighting", Open, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::End();
		return;
	}

	if (Renderer && ImGui::CollapsingHeader("Shadow Temporal", ImGuiTreeNodeFlags_DefaultOpen))
	{
		DrawShadowTemporal(*Renderer);
	}

	ImGui::End();
}

}
