#include "EnemyObject.h"

#include <Assets/MeshManager.h>
#include <Object/MeshComponent.h>
#include <Shared/FileUtils/PathUtils.h>

void EnemyObject_c::OnConstruct()
{
	Super::OnConstruct();

	MeshComponent = AddComponent<MeshComponent_c>();
	MeshComponent->SetMesh(MeshManager::RequestMesh(AssetPath_s(L"Meshes/SimpleCube.hp_mdl")));
	MeshComponent->SetCollidable(false);
}
