#include "SelectionCursorObject.h"

#include <Assets/MeshManager.h>
#include <Core/GameApp.h>
#include <Input/Input.h>
#include <Object/CameraComponent.h>
#include <Object/MeshComponent.h>
#include <Shared/FileUtils/PathUtils.h>
#include <Space/Space.h>

float3 Offsets[4] =
{
	float3(1.0f, 0.0f, 1.0f),
	float3(1.0f, 0.0f, -1.0f),
	float3(-1.0f, 0.0f, -1.0f),
	float3(-1.0f, 0.0f, 1.0f),	
};

void SelectionCursorComponent_c::OnConstruct()
{
	Super::OnConstruct();

	SpatialOwner = dynamic_cast<SpatialObject_c*>(GetOwner());

	if (!SpatialOwner)
		return;

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
		MeshComp[It] = GetOwner()->AddComponent<MeshComponent_c>(true);
		MeshComp[It]->SetMesh(MeshManager::RequestMesh(MeshAssetPath));
		MeshComp[It]->SetVisible(false);
		MeshComp[It]->SetCollidable(false);
		MeshComp[It]->SetPosition(Offsets[It]);
		MeshComp[It]->SetRotation(float3(0.0f, ConvertToRadians(Rotations[It]), 0.0f));
		MeshComp[It]->SetScale(0.01f);

		MeshComp[It]->OnCreate();
	}	
}

void SelectionCursorComponent_c::Update(float Delta)
{
	Super::Update(Delta);

	if (!SpatialOwner)
		return;

	UnsetHit();

	const Space_c* Space = GetSpace();
	if (!Space)
		return;

	if (const CameraComponent_c* Camera = Space->GetCamera())
	{
		float3 Start, End;
		Camera->CalculateRayForScreenPosition(Input::GetMousePosition(), Start, End);

		IntersectionCtx_s Trace = IntersectionCtx_s::CreateLineSegmentTrace(Start, End, true);
		Space->Trace(Trace);

		if (Trace.HasHit())
		{
			const float3 HitPosition = Trace.GetHit().Location;		

			SpatialOwner->SetPosition(HitPosition);
			SetHit(HitPosition, 0.1f);
		}
	}
}

void SelectionCursorComponent_c::SetHit(float3 Location, float InScale)
{	
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

void SelectionCursorComponent_c::UnsetHit()
{
	for (uint32_t It = 0; It < 4; ++It)
	{
		if (MeshComp[It])
		{
			MeshComp[It]->SetVisible(false);
		}
	}
}


void SelectionCursorObject_c::OnConstruct()
{
	Super::OnConstruct();

	CursorComp = AddComponent<SelectionCursorComponent_c>();
}

