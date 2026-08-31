#include "SimpleGameApp.h"

#include "Components/FighterControllerComponent.h"
#include "Objects/SelectionCursorObject.h"

#include <Input/Input.h>
#include <Object/DebugCameraObject.h>
#include <Object/CameraComponent.h>
#include <Physics/IPhysical.h>
#include <Shared/FileUtils/PathUtils.h>
#include <Shared/Logging/Logging.h>
#include <Space/Space.h>

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

	Path_s Path = Path_s(PathDirectory_e::Assets, L"Levels/ColonyTest.hp_lvl");

	if (Space)
	{
		Space->LoadLevel(Path);

		SelectionCursor = Space->CreateObject<SelectionCursorObject_c>();
	}
}

void SimpleGameApp_c::PreUpdate()
{
	GameApp_c::PreUpdate();

	if (Space && SelectionCursor)
	{
		SelectionCursor->UnsetHit();
		if (const CameraComponent_c* Camera = Space->GetCamera())
		{
			IntersectionCtx_s Trace = IntersectionCtx_s::CreateLineTrace(Camera->GetWorldPosition(), Camera->GetWorldForward(), 1000.0f);
			Space->Trace(Trace);

			if (Trace.HasHit())
			{
				const float3 HitPosition = Trace.GetHit().Location;

				SelectionCursor->SetHit(HitPosition);
			}
		}
	}
	

	if (Input::IsKeyPressed(KeyCode_e::_F8))
	{
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
