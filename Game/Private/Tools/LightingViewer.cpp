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
	ImGui::SliderFloat("Min confidence for temporal variance", &Settings.MinConfidenceForTemporalVariance, 0.0f, 0.99f, "%.2f");

	if (ImGui::Button("Reset history"))
	{
		Renderer.GetShadowDenoiser().ResetHistory();
	}
}

void DrawVisualise(SpaceRenderer_c& Renderer)
{
	static const char* kVisualiseNames[] = { "Shadow", "Variance", "Confidence" };

	int32_t Visualise = static_cast<int32_t>(Renderer.ShadowDenoise.Visualise);
	if (ImGui::Combo("Shadow", &Visualise, kVisualiseNames, IM_ARRAYSIZE(kVisualiseNames)))
	{
		Renderer.ShadowDenoise.Visualise = static_cast<ShadowVisualise_e>(Visualise);
	}

	if (Renderer.DebugViewMode != DebugViewMode_e::Lighting)
	{
		ImGui::TextDisabled("Only applies in the Lighting view mode");
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

	if (Renderer && ImGui::CollapsingHeader("Visualise", ImGuiTreeNodeFlags_DefaultOpen))
	{
		DrawVisualise(*Renderer);
	}

	ImGui::End();
}

}
