#include "SelectionCursorObject.h"

#include <Assets/MeshManager.h>
#include <Object/MeshComponent.h>
#include <Shared/FileUtils/PathUtils.h>

float3 Offsets[4] =
{
	float3(1.0f, 0.0f, 1.0f),
	float3(1.0f, 0.0f, -1.0f),
	float3(-1.0f, 0.0f, -1.0f),
	float3(-1.0f, 0.0f, 1.0f),	
};

void SelectionCursorObject_c::OnConstruct()
{
	Super::OnConstruct();

	float Rotations[4] =
	{
		0.0f,
		90.0f,
		180.0f,
		270.0f
	};

	const Path_s MeshAssetPath = Path_s(PathDirectory_e::Assets, L"Meshes/SelectionCorner.hp_mdl");
	for (uint32_t It = 0; It < 4; ++It)
	{
		MeshComp[It] = AddComponent<MeshComponent_c>(true);
		MeshComp[It]->SetMesh(MeshManager::RequestMesh(MeshAssetPath));
		MeshComp[It]->SetVisible(false);
		MeshComp[It]->SetCollidable(false);
		MeshComp[It]->SetPosition(Offsets[It]);
		MeshComp[It]->SetRotation(float3(0.0f, ConvertToRadians(Rotations[It]), 0.0f));
		MeshComp[It]->SetScale(0.01f);

		MeshComp[It]->OnCreate();
	}
}

void SelectionCursorObject_c::SetHit(float3 Location, float InScale)
{
	SetPosition(Location);
	Scale = InScale;
	for (uint32_t It = 0; It < 4; ++It)
	{
		if (MeshComp[It])
		{
			MeshComp[It]->SetPosition(Offsets[It] * Scale);
			MeshComp[It]->SetVisible(true);
		}
	}
}

void SelectionCursorObject_c::UnsetHit()
{
	for (uint32_t It = 0; It < 4; ++It)
	{
		if (MeshComp[It])
		{
			MeshComp[It]->SetVisible(false);
		}
	}
}
