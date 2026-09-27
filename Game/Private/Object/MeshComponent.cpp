#include "Object/MeshComponent.h"

#include "Assets/MeshManager.h"
#include "Space/Space.h"
#include "Physics/Intersection.h"
#include "Rendering/Mesh.h"
#include "Rendering/SpaceRenderer.h"

#include <Render/Render.h>
#include <Shared/FileUtils/JsonValue.h>
#include <Shared/FileUtils/PathUtils.h>

void MeshComponent_c::OnCreate()
{
	Super::OnCreate();

	if (Space_c* Space = GetSpace())
	{
		Space->RegisterPhysical(this);
		Space->RegisterRenderable(this);
	}
}

void MeshComponent_c::Deserialize(const JsonValue_s& Data)
{
	SpatialObjectComponent_c::Deserialize(Data);

	Path_s MeshAssetPath;
	if (JsonHelpers::ParsePath(Data, "MeshAssetPath", MeshAssetPath))
	{
		SetMesh(MeshManager::RequestMesh(MeshAssetPath));
	}
}

void MeshComponent_c::PreDestroy()
{
	if (Space_c* Space = GetSpace())
	{
		Space->UnregisterPhysical(this);
		Space->UnregisterRenderable(this);
	}
	Super::PreDestroy();
}

void MeshComponent_c::OnTransformed()
{
	if (Space_c* Space = GetSpace())
	{
		Space->DirtyRenderScene();
	}
}

void MeshComponent_c::Render(SpatialRenderingCollector_s& Collector)
{
	if (Visible && Mesh)
	{
		ObjectUniforms_s Uniforms = {};
		const bool Mirrored = MakeObjectUniforms(GetWorldMatrix(), Uniforms);

		Mesh->Render(Collector, Collector.Alloc(Uniforms), Mirrored);
	}
}

void MeshComponent_c::CollectRaytracingInstances(std::vector<rl::RaytracingInstance>& OutInstances)
{
	// A valid RTGeom was either built on an earlier frame or is being built with this scene
	if (!Visible || !Mesh || !Mesh->RTGeom)
		return;

	rl::RaytracingInstance& Instance = OutInstances.emplace_back();
	Instance.Geometry = Mesh->RTGeom;

	// Row vectors, so the instance's column vector 3x4 is the transposed upper 4x3
	const matrix3x4 Transform = MakeMatrix3x4(TransposeMatrix(GetWorldMatrix()));
	memcpy(Instance.Transform, Transform.m, sizeof(Instance.Transform));

	// TODO RT: mirrored instances need TRIANGLE_FRONT_COUNTERCLOCKWISE once rays are traced
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

	if (Space_c* Space = GetSpace())
	{
		Space->DirtyRenderScene();
	}
}

void MeshComponent_c::SetVisible(bool InVisible)
{
	if (Visible == InVisible)
		return;

	Visible = InVisible;

	if (Space_c* Space = GetSpace())
	{
		Space->DirtyRenderScene();
	}
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