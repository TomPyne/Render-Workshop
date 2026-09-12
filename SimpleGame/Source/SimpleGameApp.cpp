#include "SimpleGameApp.h"

#include "Components/FighterControllerComponent.h"
#include "Modes/BuildingMode.h"
#include "Modes/ColonyMode.h"
#include "Objects/SelectionCursorObject.h"

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

	Space->RegisterComponentClass<FighterControllerComponent_c>();

	Space->RegisterObjectClass<SelectionCursorObject_c>();
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
