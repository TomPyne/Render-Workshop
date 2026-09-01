#include "BuildingObject.h"

#include <Assets/MeshManager.h>
#include <Object/MeshComponent.h>
#include <Shared/FileUtils/JsonValue.h>
#include <Shared/FileUtils/PathUtils.h>

void BuildingObject_c::Deserialize(const JsonValue_s& Data)
{
	Super::Deserialize(Data);

	Path_s MeshAssetPath;
	if (MeshComponent && JsonHelpers::ParsePath(Data, "MeshAssetPath", MeshAssetPath))
	{
		MeshComponent->SetMesh(MeshManager::RequestMesh(MeshAssetPath));
	}	
}

void BuildingObject_c::OnConstruct()
{
	Super::OnConstruct();

	MeshComponent = AddComponent<MeshComponent_c>();
}
