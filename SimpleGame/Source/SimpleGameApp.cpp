#include "SimpleGameApp.h"

#include "Components/FighterControllerComponent.h"
#include "Input/Input.h"
#include "Levels/SimpleLevel.h"

#include <Game/Public/Object/DebugCameraObject.h>
#include <Game/Public/Object/CameraComponent.h>
#include <Shared/FileUtils/PathUtils.h>
#include <Shared/Logging/Logging.h>

void SimpleGameApp_c::RegisterClasses()
{
	GameApp_c::RegisterClasses();

	if (!Space)
		return;

	Space->RegisterComponentClass<FighterControllerComponent_c>();
}

void SimpleGameApp_c::Load()
{
	GameApp_c::Load();

	Path_s Path = Path_s(PathDirectory_e::Assets, L"Levels/FlightTest.hp_lvl");

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
		LOGINFO("Debug cam toggled");
		ToggleDebugCamera(DebugCamera == nullptr);
	}
}

void SimpleGameApp_c::ToggleDebugCamera(bool Enabled)
{
	if (!Space)
		return;

	ASSERTMSG(Enabled == (DebugCamera == nullptr), "[ToggleDebugCamera] Toggle mismatch");

	if (Enabled)
	{
		float3 Position = float3(0.0f);
		float3 Rotation = float3(0.0f);
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
		Space->DestroyObject(DebugCamera.get());
		DebugCamera = {};
	}
}
