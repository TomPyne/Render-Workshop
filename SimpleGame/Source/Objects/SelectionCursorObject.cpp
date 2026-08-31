#include "SelectionCursorObject.h"

#include <Assets/MeshManager.h>
#include <Object/MeshComponent.h>
#include <Shared/FileUtils/PathUtils.h>

void SelectionCursorObject_c::OnConstruct()
{
	Super::OnConstruct();

	MeshComp = AddComponent<MeshComponent_c>(true);

	const Path_s MeshAssetPath = Path_s(PathDirectory_e::Assets, L"Meshes/SelectionCorner.hp_mdl");
	MeshComp->SetMesh(MeshManager::RequestMesh(MeshAssetPath));
	MeshComp->SetVisible(false);
	MeshComp->SetCollidable(false);
	MeshComp->SetScale(0.01f);

	MeshComp->OnCreate();
}

void SelectionCursorObject_c::SetHit(float3 Location)
{
	SetPosition(Location);
	if (MeshComp)
	{
		MeshComp->SetVisible(true);
	}
}

void SelectionCursorObject_c::UnsetHit()
{
	if (MeshComp)
	{
		MeshComp->SetVisible(false);
	}
}
