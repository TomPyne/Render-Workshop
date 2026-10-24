#include "SimpleGameApp.h"

#include "Components/AIGymFlowComponent.h"
#include "Components/CircleMoverComponent.h"
#include "Components/DeathVolumeComponent.h"
#include "Components/HovercraftControllerComponent.h"
#include "Components/HovercraftGymFlowComponent.h"
#include "Components/LandingPadComponent.h"
#include "Components/NavObstacleComponent.h"
#include "Components/NestComponent.h"
#include "Materials/SimpleGameMaterials.h"
#include "Modes/ColonyMode.h"
#include "Objects/EnemyObject.h"
#include "Objects/HovercraftObject.h"
#include "Objects/SelectionCursorObject.h"

#include <Assets/MaterialManager.h>
#include <Input/Input.h>
#include <Object/DebugCameraObject.h>
#include <Object/CameraComponent.h>
#include <Shared/FileUtils/PathUtils.h>
#include <Shared/Logging/Logging.h>
#include <Space/Space.h>
#include <Tools/PerfStats.h>

#include <RenderImGui/imgui/imgui.h>

namespace Mode_e
{
	enum Type
	{
		NONE,
		COUNT
	};
}


static struct
{
	bool ShowUI = true;
	bool ShowPerfWindow = true;
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
	Space->RegisterObjectClass<EnemyObject_c>();
	Space->RegisterObjectClass<HovercraftObject_c>();
	Space->RegisterObjectClass<SelectionCursorObject_c>();

	// Components
	Space->RegisterComponentClass<AIGymFlowComponent_c>();
	Space->RegisterComponentClass<CircleMoverComponent_c>();
	Space->RegisterComponentClass<DeathVolumeComponent_c>();
	Space->RegisterComponentClass<HovercraftControllerComponent_c>();
	Space->RegisterComponentClass<HovercraftGymFlowComponent_c>();
	Space->RegisterComponentClass<LandingPadComponent_c>();
	Space->RegisterComponentClass<NavObstacleComponent_c>();
	Space->RegisterComponentClass<NestComponent_c>();

	// Materials
	RegisterMaterials();
}

void SimpleGameApp_c::Load()
{
	GameApp_c::Load();

	G.ColonyModes[Mode_e::NONE] = nullptr;

	if (Space)
	{
		//Space->LoadLevel(AssetPath_s(L"Levels/Lighting.hp_lvl"));
		//Space->LoadLevel(AssetPath_s(L"Levels/HovercraftGym.hp_lvl"));
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
		if (ImGui::MenuItem("Performance", nullptr, G.ShowPerfWindow))
		{
			G.ShowPerfWindow = !G.ShowPerfWindow;
		}
		DrawViewModeMenu();
		ImGui::EndMainMenuBar();
	}

	if (G.ColonyMode)
	{
		G.ColonyMode->ImGuiUpdate();
	}

	if (G.ShowPerfWindow)
	{
		PerfStats::DrawPerfWindow(&G.ShowPerfWindow);
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
