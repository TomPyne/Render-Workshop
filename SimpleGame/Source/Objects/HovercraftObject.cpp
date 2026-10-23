#include "HovercraftObject.h"

#include "Components/GameFlowComponent.h"
#include "Components/HovercraftControllerComponent.h"
#include "Objects/SelectionCursorObject.h"

#include <Assets/MeshManager.h>
#include <Object/CameraComponent.h>
#include <Object/MeshComponent.h>
#include <Shared/FileUtils/PathUtils.h>
#include <Space/Space.h>

void HovercraftObject_c::OnConstruct()
{
	Super::OnConstruct();

	MeshComponent = AddComponent<MeshComponent_c>();
	MeshComponent->SetMesh(MeshManager::RequestMesh(AssetPath_s(L"Meshes/Hovercraft.hp_mdl")));
	MeshComponent->SetCollidable(false);

	CameraComponent = AddComponent<CameraComponent_c>();
	ControllerComponent = AddComponent<HovercraftControllerComponent_c>();
}

void HovercraftObject_c::OnCreate()
{
	Super::OnCreate();

	if (Space_c* Space = GetSpace())
	{
		TargetCursor = Space->CreateObject<SelectionCursorObject_c>();
	}
}

void HovercraftObject_c::OnDestroy()
{
	if (Space_c* Space = GetSpace())
	{
		Space->DestroyObject(TargetCursor.get());
	}
	TargetCursor = nullptr;

	Super::OnDestroy();
}

void HovercraftObject_c::Die()
{
	if (Dead)
		return;

	Dead = true;

	MeshComponent->SetVisible(false);

	if (TargetCursor)
	{
		TargetCursor->UnsetHit();
	}

	if (std::shared_ptr<GameFlowComponent_c> FlowShared = Flow.lock())
	{
		FlowShared->OnHovercraftDied(*this);
	}
}

void HovercraftObject_c::Respawn(float3 Position, float Yaw)
{
	Dead = false;

	MeshComponent->SetVisible(true);

	ControllerComponent->ResetTo(Position, Yaw);

	ResetMotion();
}
