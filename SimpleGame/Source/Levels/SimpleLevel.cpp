#include "SimpleLevel.h"

#include <Game/Object/CameraComponent.h>
#include <Game/Object/FlyControllerComponent.h>
#include <Game/Object/RuntimeMeshComponent.h>
#include <Game/Rendering/Materials.h>
#include <Shared/ModelUtils/PlaneBuilder.h>

void SimpleLevel_c::Load()
{
	if (std::shared_ptr<SpatialObject_c> CameraObject = AddSpatialComponentToLevel<CameraComponent_c>())
	{
		CameraObject->AddComponent<FlyControllerComponent_c>();
		CameraObject->SetPosition(float3(0, 5, -10));
	}

	if (std::shared_ptr<RuntimeMeshObject_c> FloorMeshObject = AddObjectToLevel<RuntimeMeshObject_c>())
	{
		if (FloorMeshObject->MeshComponent)
		{
			PlaneBuilder::PlaneMesh_s<uint32_t> PlaneMesh = PlaneBuilder::BuildPlaneMesh32(float2(1000.0f, 1000.0f));

			RuntimeMeshDesc_s FloorMeshDesc;
			FloorMeshDesc.Positions = &PlaneMesh.Positions;
			FloorMeshDesc.Indices = &PlaneMesh.Indices;

			FloorMeshObject->MeshComponent->UpdateMesh(FloorMeshDesc);

			// Set material

			FloorMeshObject->MeshComponent->SetMaterial(MakeBasicMaterial(float3(1, 0, 0)));
		}
	}
}
