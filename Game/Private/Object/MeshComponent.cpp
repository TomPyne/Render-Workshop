#include "Object/MeshComponent.h"

#include "Assets/MeshManager.h"
#include "Physics/Intersection.h"
#include "Rendering/Mesh.h"

#include <Render/Render.h>
#include <Shared/FileUtils/JsonValue.h>
#include <Shared/FileUtils/PathUtils.h>


void MeshComponent_c::Deserialize(const JsonValue_s& Data)
{
	SpatialObjectComponent_c::Deserialize(Data);

	Path_s MeshAssetPath;
	if (JsonHelpers::ParsePath(Data, "MeshAssetPath", MeshAssetPath))
	{
		SetMesh(MeshManager::RequestMesh(MeshAssetPath));
	}
}

void MeshComponent_c::Render(SpatialRenderingCollector_s& Collector)
{
	if (Visible && Mesh)
	{
		Mesh->Render(Collector, rl::CreateDynamicConstantBuffer(&GetWorldMatrix()));
	}
}

void MeshComponent_c::Intersect(IntersectionCtx_s& Context) const
{
	if (!Collidable)
		return;

	// Sphere and box traces land in a later phase.
	if (!Mesh || Context.Shape != TraceShape_e::Line)
	{
		return;
	}

	const CandidateSpace_s Space = MakeCandidateSpace(GetWorldMatrix());

	Hit_s Hit;
	Hit.T = Context.BestT();

	if (TraceLineMesh(Context.GetLineTrace(), *Mesh, Space, Context.Params, Hit))
	{
		Context.SetClosestHit(this, Hit);
	}
}

void MeshComponent_c::SetMesh(const std::shared_ptr<struct Mesh_s>& InMesh)
{
	Mesh = InMesh;
}

uint32_t MeshComponent_c::GetMaterialCount() const
{
	return Mesh ? static_cast<uint32_t>(Mesh->Surfaces.size()) : 0;
}

MaterialShaderInstance_c* MeshComponent_c::GetMaterial(uint32_t Index) const
{
	if (Index < GetMaterialCount())
	{
		return Mesh->Surfaces[Index].Material.get();
	}
	return nullptr;
}

void MeshObject_c::OnConstruct()
{
	SpatialObject_c::OnConstruct();

	MeshComponent = AddComponent<MeshComponent_c>();
}

void MeshObject_c::Deserialize(const JsonValue_s& Data)
{
	SpatialObject_c::Deserialize(Data);

	if (MeshComponent)
	{
		Path_s MeshAssetPath;
		if (JsonHelpers::ParsePath(Data, "MeshAssetPath", MeshAssetPath))
		{
			MeshComponent->SetMesh(MeshManager::RequestMesh(MeshAssetPath));
		}
	}
}