#include "Tools/PerfStats.h"

#include <RenderImGui/imgui/imgui.h>

#include <algorithm>
#include <cfloat>
#include <chrono>
#include <functional>

namespace
{
	using HighResolutionClock = std::chrono::high_resolution_clock;

	constexpr int FrameHistoryCount = 240;

	struct
	{
		float FrameMs[FrameHistoryCount] = {};
		int FrameIndex = 0;
		int FrameCount = 0;

		float UpdateMs = 0.0f;
		float RenderMs = 0.0f;

		HighResolutionClock::time_point UpdateStart;
		HighResolutionClock::time_point RenderStart;
	} G;

	float MsSince(HighResolutionClock::time_point Start)
	{
		return std::chrono::duration<float, std::milli>(HighResolutionClock::now() - Start).count();
	}
}

void PerfStats::RecordFrame(float DeltaSeconds)
{
	G.FrameMs[G.FrameIndex] = DeltaSeconds * 1000.0f;
	G.FrameIndex = (G.FrameIndex + 1) % FrameHistoryCount;
	G.FrameCount = std::min(G.FrameCount + 1, FrameHistoryCount);
}

void PerfStats::BeginUpdate()
{
	G.UpdateStart = HighResolutionClock::now();
}

void PerfStats::EndUpdate()
{
	G.UpdateMs = MsSince(G.UpdateStart);
}

void PerfStats::BeginRender()
{
	G.RenderStart = HighResolutionClock::now();
}

void PerfStats::EndRender()
{
	G.RenderMs = MsSince(G.RenderStart);
}

void PerfStats::DrawPerfWindow(bool* Open)
{
	if (G.FrameCount == 0)
		return;

	float Sum = 0.0f;
	float MinMs = FLT_MAX;
	float MaxMs = 0.0f;
	float Sorted[FrameHistoryCount];
	for (int i = 0; i < G.FrameCount; ++i)
	{
		Sum += G.FrameMs[i];
		MinMs = std::min(MinMs, G.FrameMs[i]);
		MaxMs = std::max(MaxMs, G.FrameMs[i]);
		Sorted[i] = G.FrameMs[i];
	}
	const float AvgMs = Sum / G.FrameCount;

	const int OnePercentIndex = G.FrameCount / 100;
	std::nth_element(Sorted, Sorted + OnePercentIndex, Sorted + G.FrameCount, std::greater<float>());
	const float OnePercentLowMs = Sorted[OnePercentIndex];

	ImGui::SetNextWindowBgAlpha(0.7f);
	if (ImGui::Begin("Performance", Open, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("FPS: %.1f (%.2f ms)", AvgMs > 0.0f ? 1000.0f / AvgMs : 0.0f, AvgMs);
		ImGui::Text("Min / Max: %.2f / %.2f ms", MinMs, MaxMs);
		ImGui::Text("1%% low: %.1f FPS", OnePercentLowMs > 0.0f ? 1000.0f / OnePercentLowMs : 0.0f);
		ImGui::Separator();
		ImGui::Text("CPU Update: %.2f ms", G.UpdateMs);
		ImGui::Text("CPU Render: %.2f ms (prev frame)", G.RenderMs);
		ImGui::PlotLines("##FrameTimes", G.FrameMs, FrameHistoryCount, G.FrameIndex, "Frame ms", 0.0f, std::max(33.3f, MaxMs), ImVec2(260, 60));
	}
	ImGui::End();
}
