#include "SimpleGameApp.h"

#include "Components/FighterControllerComponent.h"
#include "Objects/SelectionCursorObject.h"

#include <Input/Input.h>
#include <Object/DebugCameraObject.h>
#include <Object/CameraComponent.h>
#include <Physics/IPhysical.h>
#include <Render/Render.h>
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
			const float ScreenWidth = static_cast<float>(MainRenderView->Width);
			const float ScreenHeight = static_cast<float>(MainRenderView->Height);
			float2 MousePosition = Input::GetMousePosition();

			float2 NDC = float2(((MousePosition.x + 0.5f) / ScreenWidth) * 2.0f - 1.0f,
				1.0f - ((MousePosition.y + 0.5f) / ScreenHeight) * 2.0f);

			matrix ProjectionMatrix = Camera->CalculateProjectionMatrix(MainRenderView->Width, MainRenderView->Height);
			matrix ViewMatrix = Camera->CalculateViewMatrix();
			matrix InverseViewProjection = InverseMatrix(ViewMatrix * ProjectionMatrix);

			const float4 Near = TransformF4(float4(NDC.x, NDC.y, 0.0f, 1.0f), InverseViewProjection);
			const float4 Far = TransformF4(float4(NDC.x, NDC.y, 1.0f, 1.0f), InverseViewProjection);

			IntersectionCtx_s Trace = IntersectionCtx_s::CreateLineSegmentTrace(Near.xyz / Near.w, Far.xyz / Far.w, true);
			Space->Trace(Trace);

			if (Trace.HasHit())
			{
				const float3 HitPosition = Trace.GetHit().Location;

				SelectionCursor->SetHit(HitPosition, 0.1f);
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
