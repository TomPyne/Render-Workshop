#include "SimpleLevel.h"

#include <Game/Object/RuntimeMeshComponent.h>
#include <Shared/ModelUtils/PlaneBuilder.h>

void SimpleLevel_c::Load()
{
	// Add floor object

	if (std::shared_ptr<RuntimeMeshObject_c> FloorMeshObject = AddObjectToLevel<RuntimeMeshObject_c>())
	{
		if (FloorMeshObject->MeshComponent)
		{
			PlaneBuilder::PlaneMesh_s<uint32_t> PlaneMesh = PlaneBuilder::BuildPlaneMesh32(float2(10.0f, 10.0f));

			RuntimeMeshDesc_s FloorMeshDesc;
			FloorMeshDesc.Positions = &PlaneMesh.Positions;
			FloorMeshDesc.Indices = &PlaneMesh.Indices;

			FloorMeshObject->MeshComponent->UpdateMesh(FloorMeshDesc);

			// Set material
		}
	}
	// Add some objects
}
