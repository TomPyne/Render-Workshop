#include "Tools/DistanceFieldViewer.h"

#include "Assets/AssetManager.h"
#include "Rendering/Mesh.h"

#include <RenderImGui/imgui/imgui.h>
#include <SurfMath.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

namespace DistanceFieldViewer
{
namespace
{

struct
{
	std::string SelectedMesh;
	int32_t Axis = 2;
	int32_t Slice = 0;
} G;

constexpr float kSliceViewSize = 512.0f;
constexpr float kContourSpacingVoxels = 4.0f;

float DecodeDistance(const SignedDistanceField_s& SDF, uint8_t Value)
{
	return (static_cast<float>(Value) - 128.0f) / 127.0f * SDF.MaxDistance;
}

ImU32 DistanceColour(float Distance, float MaxDistance, float VoxelSize)
{
	if (fabsf(Distance) < VoxelSize * 0.5f)
	{
		return IM_COL32(255, 255, 255, 255);
	}

	const float3 Base = Distance < 0.0f ? float3(0.25f, 0.55f, 1.0f) : float3(1.0f, 0.6f, 0.2f);
	const float Falloff = 1.0f - 0.6f * Min(fabsf(Distance) / MaxDistance, 1.0f);
	const float Contour = 0.8f + 0.2f * cosf(2.0f * K_PI * Distance / (VoxelSize * kContourSpacingVoxels));
	const float3 Colour = Base * (Falloff * Contour);

	return IM_COL32(static_cast<int>(Colour.x * 255.0f), static_cast<int>(Colour.y * 255.0f), static_cast<int>(Colour.z * 255.0f), 255);
}

void DrawSlice(const SignedDistanceField_s& SDF)
{
	const uint3 Dims = SDF.Dims;
	const float VoxelSize = (SDF.VolumeBounds.maxs.x - SDF.VolumeBounds.mins.x) / Dims.x;

	static const char* kAxisNames[] = { "X", "Y", "Z" };
	ImGui::Combo("Slice Axis", &G.Axis, kAxisNames, 3);

	// The slice is drawn with U to the right and V up, picked so Y stays up where it can.
	const uint32_t SliceAxis = static_cast<uint32_t>(G.Axis);
	const uint32_t UAxis = SliceAxis == 0 ? 2 : 0;
	const uint32_t VAxis = SliceAxis == 1 ? 2 : 1;

	G.Slice = std::clamp(G.Slice, 0, static_cast<int32_t>(Dims.v[SliceAxis]) - 1);
	ImGui::SliderInt("Slice", &G.Slice, 0, static_cast<int32_t>(Dims.v[SliceAxis]) - 1);

	const float SliceCoord = SDF.VolumeBounds.mins.v[SliceAxis] + (G.Slice + 0.5f) * VoxelSize;
	ImGui::Text("Slice at %s = %.3f, U = %s, V = %s", kAxisNames[SliceAxis], SliceCoord, kAxisNames[UAxis], kAxisNames[VAxis]);

	const uint32_t DimU = Dims.v[UAxis];
	const uint32_t DimV = Dims.v[VAxis];
	const float CellSize = Max(2.0f, floorf(kSliceViewSize / static_cast<float>(Max(DimU, DimV))));

	const ImVec2 Origin = ImGui::GetCursorScreenPos();
	const ImVec2 Size = ImVec2(CellSize * DimU, CellSize * DimV);
	ImGui::InvisibleButton("SDFSlice", Size);

	const auto VoxelAt = [&](uint32_t U, uint32_t V)
	{
		uint3 Coord;
		Coord.v[SliceAxis] = static_cast<uint32_t>(G.Slice);
		Coord.v[UAxis] = U;
		Coord.v[VAxis] = V;
		return SDF.Voxels[Coord.x + static_cast<size_t>(Coord.y) * Dims.x + static_cast<size_t>(Coord.z) * Dims.x * Dims.y];
	};

	ImDrawList* DrawList = ImGui::GetWindowDrawList();
	for (uint32_t V = 0; V < DimV; V++)
	{
		const float Top = Origin.y + (DimV - 1 - V) * CellSize;
		for (uint32_t U = 0; U < DimU; U++)
		{
			const float Left = Origin.x + U * CellSize;
			const float Distance = DecodeDistance(SDF, VoxelAt(U, V));
			DrawList->AddRectFilled(ImVec2(Left, Top), ImVec2(Left + CellSize, Top + CellSize), DistanceColour(Distance, SDF.MaxDistance, VoxelSize));
		}
	}

	if (ImGui::IsItemHovered())
	{
		const ImVec2 Mouse = ImGui::GetIO().MousePos;
		const int32_t U = static_cast<int32_t>((Mouse.x - Origin.x) / CellSize);
		const int32_t V = static_cast<int32_t>(DimV) - 1 - static_cast<int32_t>((Mouse.y - Origin.y) / CellSize);
		if (U >= 0 && U < static_cast<int32_t>(DimU) && V >= 0 && V < static_cast<int32_t>(DimV))
		{
			const uint8_t Value = VoxelAt(U, V);
			ImGui::SetTooltip("%s %d, %s %d\nValue %u\nDistance %.4f", kAxisNames[UAxis], U, kAxisNames[VAxis], V, Value, DecodeDistance(SDF, Value));
		}
	}
}

}

void DrawWindow(bool* Open)
{
	if (!ImGui::Begin("Distance Fields", Open, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::End();
		return;
	}

	std::vector<std::shared_ptr<Mesh_s>> Meshes;
	AssetManager_c::GetLoadedMeshes(Meshes);
	std::sort(Meshes.begin(), Meshes.end(), [](const std::shared_ptr<Mesh_s>& A, const std::shared_ptr<Mesh_s>& B) { return A->Name < B->Name; });

	const Mesh_s* Selected = nullptr;
	float TotalKB = 0.0f;

	if (ImGui::BeginListBox("##Meshes", ImVec2(kSliceViewSize, 200.0f)))
	{
		for (const std::shared_ptr<Mesh_s>& Mesh : Meshes)
		{
			SignedDistanceField_s* SDF = Mesh->SDF.get();
			char Label[256];
			if (SDF == nullptr || SDF->Voxels.empty())
			{
				snprintf(Label, sizeof(Label), "%s  (none)", Mesh->Name.c_str());
				ImGui::TextDisabled("%s", Label);
				continue;
			}

			const float KB = SDF->Voxels.size() / 1024.0f;
			TotalKB += KB;

			snprintf(Label, sizeof(Label), "%s  %ux%ux%u  %.0fKB", Mesh->Name.c_str(), SDF->Dims.x, SDF->Dims.y, SDF->Dims.z, KB);
			const bool IsSelected = Mesh->Name == G.SelectedMesh;
			if (ImGui::Selectable(Label, IsSelected))
			{
				G.SelectedMesh = Mesh->Name;
			}

			if (IsSelected)
			{
				Selected = Mesh.get();
			}
		}
		ImGui::EndListBox();
	}

	ImGui::Text("Total %.0fKB", TotalKB);

	if (Selected)
	{
		ImGui::Separator();
		ImGui::Text("Max distance %.3f", Selected->SDF->MaxDistance);
		DrawSlice(*Selected->SDF);
	}

	ImGui::End();
}

}
