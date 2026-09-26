#include "SimpleGameApp.h"

#include "Components/FighterControllerComponent.h"
#include "Materials/SunTempleMaterials.h"
#include "Modes/BuildingMode.h"
#include "Modes/ColonyMode.h"
#include "Objects/SelectionCursorObject.h"

#include <Assets/MaterialManager.h>
#include <Input/Input.h>
#include <Object/DebugCameraObject.h>
#include <Object/CameraComponent.h>
#include <Shared/FileUtils/PathUtils.h>
#include <Shared/Logging/Logging.h>
#include <Space/Space.h>

#include <RenderImGui/imgui/imgui.h>

namespace Mode_e
{
	enum Type
	{
		NONE,
		BUILDING,
		COUNT
	};
}


static struct
{
	bool ShowUI = true;
	Mode_e::Type CurrentMode = Mode_e::NONE;
	ColonyMode_c* ColonyMode = nullptr;

	std::unique_ptr<ColonyMode_c> ColonyModes[Mode_e::COUNT] = { nullptr };
} G;

static void SwitchMode(Mode_e::Type NewMode)
{
	if (G.CurrentMode == NewMode)
		return;

	if (G.ColonyMode)
	{
		G.ColonyMode->Exit();
	}

	G.ColonyMode = G.ColonyModes[NewMode].get();

	if (G.ColonyMode)
	{
		G.ColonyMode->Enter();
	}

	G.CurrentMode = NewMode;
}

void SimpleGameApp_c::RegisterClasses()
{
	GameApp_c::RegisterClasses();

	if (!Space)
		return;

	// Objects
	Space->RegisterObjectClass<SelectionCursorObject_c>();

	// Components
	Space->RegisterComponentClass<FighterControllerComponent_c>();

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
	MaterialManager::RegisterMaterialShaderClass<StatueMaterialShader_c>(L"StatueMaterialShader");
	MaterialManager::RegisterMaterialShaderClass<StairsMaterialShader_c>(L"StairsMaterialShader");
	MaterialManager::RegisterMaterialShaderClass<ShieldMaterialShader_c>(L"ShieldMaterialShader");
	MaterialManager::RegisterMaterialShaderClass<WaveFoamMaterialShader_c>(L"WaveFoamMaterialShader");
	MaterialManager::RegisterMaterialShaderClass<SoulTreeMaterialShader_c>(L"SoulTreeMaterialShader");
}

void SimpleGameApp_c::Load()
{
	GameApp_c::Load();

	G.ColonyModes[Mode_e::NONE] = nullptr;
	G.ColonyModes[Mode_e::BUILDING] = std::make_unique<BuildingMode_c>(Space.get());

	Path_s Path = Path_s(PathDirectory_e::Assets, L"Levels/SunTemple.hp_lvl");

	if (Space)
	{
		Space->LoadLevel(Path);
	}
}

void SimpleGameApp_c::PreUpdate()
{
	GameApp_c::PreUpdate();	

	if (Input::IsKeyPressed(KeyCode_e::_F8))
	{
		ToggleDebugCamera(DebugCamera == nullptr);
	}
}

void SimpleGameApp_c::Update(float Delta)
{
	if (G.ColonyMode)
	{
		G.ColonyMode->UpdateMode(Delta);
	}

	GameApp_c::Update(Delta);
}

void SimpleGameApp_c::ImGuiUpdate()
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
		if (ImGui::MenuItem("Building Mode"))
		{
			SwitchMode(Mode_e::BUILDING);
		}
		ImGui::EndMainMenuBar();
	}

	if (G.ColonyMode)
	{
		G.ColonyMode->ImGuiUpdate();
	}
}

void SimpleGameApp_c::ToggleDebugCamera(bool Enabled)
{
	if (!Space)
		return;

	ASSERTMSG(Enabled == (DebugCamera == nullptr), "[ToggleDebugCamera] Toggle mismatch");
	
	if (Enabled)
	{
		LOGINFO("Debug cam enabled");

		float3 Position = float3(0.0f);
		quat Rotation = quat::Identity();
		if (CameraComponent_c* CurrentCamera = Space->GetCamera())
		{
			Position = CurrentCamera->GetWorldPosition();
			Rotation = CurrentCamera->GetWorldRotation();
		}
		DebugCamera = Space->CreateObject<DebugCameraObject_c>();
		DebugCamera->SetPosition(Position);
		DebugCamera->SetRotation(Rotation);		
	}
	else
	{
		LOGINFO("Debug cam disabled");

		Space->DestroyObject(DebugCamera.get());
		DebugCamera = {};
	}
}
