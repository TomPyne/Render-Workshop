#include "SunTempleApp.h"

#include "Components/MoverComponent.h"
#include "Materials/SunTempleMaterials.h"

#include <Assets/MaterialManager.h>
#include <Assets/MeshManager.h>
#include <Object/MeshComponent.h>
#include <Object/SpatialObject.h>
#include <Shared/FileUtils/PathUtils.h>
#include <Space/Space.h>
#include <Tools/GameStats.h>
#include <Tools/PerfStats.h>

#include <RenderImGui/imgui/imgui.h>
static struct
{
	bool ShowUI = true;
	bool ShowPerfWindow = true;
	bool ShowGameWindow = true;
} G;

void SunTempleApp_c::RegisterClasses()
{
	GameApp_c::RegisterClasses();

	if (!Space)
		return;

	// Components
	Space->RegisterComponentClass<MoverComponent_c>();

	// Materials
	MaterialManager::RegisterMaterialShaderClass<ArchMaterialShader_c>(L"ArchBRDFMaterialShader");
	MaterialManager::RegisterMaterialShaderClass<BackgroundMatteMaterialShader_c>(L"BackgroundMatteMaterialShader");
	MaterialManager::RegisterMaterialShaderClass<BottomTrimMaterialShader_c>(L"BottomTrimMaterialShader");
	MaterialManager::RegisterMaterialShaderClass<TreeBranchesMaterialShader_c>(L"TreeBranchesMaterialShader");
	MaterialManager::RegisterMaterialShaderClass<TreeTrunkMaterialShader_c>(L"TreeTrunkMaterialShader");
	MaterialManager::RegisterMaterialShaderClass<TrimMaterialShader_c>(L"TrimMaterialShader");
	MaterialManager::RegisterMaterialShaderClass<StoneBrickWallMaterialShader_c>(L"StoneBrickWallMaterialShader");
	MaterialManager::RegisterMaterialShaderClass<SoulRocksMaterialShader_c>(L"SoulRocksMaterialShader");
	MaterialManager::RegisterMaterialShaderClass<DomeMaterialShader_c>(L"DomeMaterialShader");
	MaterialManager::RegisterMaterialShaderClass<FirePitMaterialShader_c>(L"FirePitMaterialShader");
	MaterialManager::RegisterMaterialShaderClass<FloorMaterialShader_c>(L"FloorTilesMaterialShader");
	MaterialManager::RegisterMaterialShaderClass<WaterMaterialShader_c>(L"WaterMaterialShader");
	MaterialManager::RegisterMaterialShaderClass<PillarMaterialShader_c>(L"PillarMaterialShader");
	MaterialManager::RegisterMaterialShaderClass<RailingMaterialShader_c>(L"RailingMaterialShader");
	MaterialManager::RegisterMaterialShaderClass<SkyMaterialShader_c>(L"SkyMaterialShader");
	MaterialManager::RegisterMaterialShaderClass<StatueMaterialShader_c>(L"StatueMaterialShader");
	MaterialManager::RegisterMaterialShaderClass<StairsMaterialShader_c>(L"StairsMaterialShader");
	MaterialManager::RegisterMaterialShaderClass<ShieldMaterialShader_c>(L"ShieldMaterialShader");
	MaterialManager::RegisterMaterialShaderClass<WaveFoamMaterialShader_c>(L"WaveFoamMaterialShader");
	MaterialManager::RegisterMaterialShaderClass<SoulTreeMaterialShader_c>(L"SoulTreeMaterialShader");
}

void SunTempleApp_c::Load()
{
	GameApp_c::Load();

	Path_s Path = Path_s(PathDirectory_e::Assets, L"Levels/SunTemple.hp_lvl");

	if (Space)
	{
		Space->LoadLevel(Path);

		if (std::shared_ptr<SpatialObject_c> MoverObject = Space->CreateObject<SpatialObject_c>())
		{
			MoverObject->SetPosition(float3(-13.8f, 6.9f, -9.2f));
			MoverObject->AddComponent<MoverComponent_c>();
			if (MeshComponent_c* MeshComp = MoverObject->AddComponent<MeshComponent_c>())
			{
				MeshComp->SetMesh(MeshManager::RequestMesh(Path_s(PathDirectory_e::Assets, L"Game", L"Meshes/Sphere.hp_mdl")));
			}
		}
	}
}

void SunTempleApp_c::ImGuiUpdate()
{
	GameApp_c::ImGuiUpdate();

	if (ImGui::IsKeyPressed(ImGuiKey_F1))
	{
		G.ShowUI = !G.ShowUI;
	}
	if (!G.ShowUI)
		return;

	if (ImGui::BeginMainMenuBar())
	{
		DrawViewModeMenu();
		if (ImGui::MenuItem("Performance", nullptr, G.ShowPerfWindow))
		{
			G.ShowPerfWindow = !G.ShowPerfWindow;
		}
		if (ImGui::MenuItem("Game", nullptr, G.ShowGameWindow))
		{
			G.ShowGameWindow = !G.ShowGameWindow;
		}
		ImGui::EndMainMenuBar();
	}

	if (G.ShowPerfWindow)
	{
		PerfStats::DrawPerfWindow(&G.ShowPerfWindow);
	}

	if (G.ShowGameWindow)
	{
		GameStats::DrawGameStatsWindow(&G.ShowGameWindow);
	}
}