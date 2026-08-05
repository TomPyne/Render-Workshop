#include "Object/MeshComponent.h"

#include "Assets/MeshManager.h"
#include "Rendering/Mesh.h"

#include <Render/Render.h>
#include <Shared/FileUtils/JsonValue.h>
#include <Shared/FileUtils/PathUtils.h>


void MeshComponent_c::Deserialize(const JsonValue_s& Data)
{
	SpatialObjectComponent_c::Deserialize(Data);

	std::wstring MeshAssetPath;
	if (JsonHelpers::ParseWString(Data, "MeshAssetPath", MeshAssetPath))
	{
		SetMesh(MeshManager::RequestMesh(MeshAssetPath));
	}
}

void MeshComponent_c::Render(SpatialRenderingCollector_s& Collector)
{
	if (Mesh)
	{
		Mesh->Render(Collector, rl::CreateDynamicConstantBuffer(&GetTransform().GetMatrix()));
	}
}

void MeshComponent_c::SetMesh(const std::shared_ptr<struct Mesh_s>& InMesh)
{
	Mesh = InMesh;
}

void MeshObject_c::Deserialize(const JsonValue_s& Data)
{
	SpatialObject_c::Deserialize(Data);

	if (MeshComponent)
	{
		std::wstring MeshAssetPath;
		if (JsonHelpers::ParseWString(Data, "MeshAssetPath", MeshAssetPath))
		{
			MeshComponent->SetMesh(MeshManager::RequestMesh(MeshAssetPath));
		}
	}
}

void MeshObject_c::OnCreate()
{
	MeshComponent = AddComponent<MeshComponent_c>();
}
