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

	static const char* kHistoryFormatNames[] = { "RGBA16 Float", "RGB10A2 Unorm" };

	int32_t HistoryFormat = static_cast<int32_t>(Settings.HistoryFormat);
	if (ImGui::Combo("History format", &HistoryFormat, kHistoryFormatNames, IM_ARRAYSIZE(kHistoryFormatNames)))
	{
		Settings.HistoryFormat = static_cast<ShadowHistoryFormat_e>(HistoryFormat);
	}

	const ShadowDenoiser_c& Denoiser = Renderer.GetShadowDenoiser();
	ImGui::Text("Shadow history %.1f MB", Denoiser.GetHistoryMemoryBytes() / 1.0e6);
	ImGui::Text("Linear depth history %.1f MB", Denoiser.GetLinearDepthHistoryMemoryBytes() / 1.0e6);

	if (ImGui::Button("Reset history"))
	{
		Renderer.GetShadowDenoiser().ResetHistory();
	}
}

void DrawShadowSpatial(SpaceRenderer_c& Renderer)
{
	ShadowDenoiseSettings_s& Settings = Renderer.ShadowDenoise;

	ImGui::Checkbox("Enabled##Spatial", &Settings.SpatialEnabled);

	int32_t Iterations = static_cast<int32_t>(Settings.SpatialIterations);
	if (ImGui::SliderInt("Iterations", &Iterations, 0, 5))
	{
		Settings.SpatialIterations = static_cast<uint32_t>(Iterations);
	}

	ImGui::Checkbox("Plane weight", &Settings.PlaneWeight);
	ImGui::SliderFloat("Depth sigma", &Settings.DepthSigma, 0.0005f, 0.1f, "%.4f", ImGuiSliderFlags_Logarithmic);
	ImGui::Checkbox("Normal weight", &Settings.NormalWeight);
	ImGui::SliderFloat("Normal power", &Settings.NormalPower, 1.0f, 256.0f, "%.0f", ImGuiSliderFlags_Logarithmic);
	ImGui::Checkbox("Variance weight", &Settings.VarianceWeight);
	ImGui::SliderFloat("Variance sigma", &Settings.VarianceSigma, 0.5f, 16.0f, "%.1f");
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

	if (Renderer && ImGui::CollapsingHeader("Shadow Spatial", ImGuiTreeNodeFlags_DefaultOpen))
	{
		DrawShadowSpatial(*Renderer);
	}

	if (Renderer && ImGui::CollapsingHeader("Visualise", ImGuiTreeNodeFlags_DefaultOpen))
	{
		DrawVisualise(*Renderer);
	}

	ImGui::End();
}

}
