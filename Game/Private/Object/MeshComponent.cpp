#include "Object/MeshComponent.h"

#include "Assets/MaterialManager.h"
#include "Assets/MeshManager.h"
#include "Space/Space.h"
#include "Physics/Intersection.h"
#include "Rendering/DistanceFieldScene.h"
#include "Rendering/Mesh.h"
#include "Rendering/SpaceRenderer.h"

#include <Render/Render.h>
#include <Shared/FileUtils/JsonValue.h>
#include <Shared/FileUtils/PathUtils.h>
#include <Shared/Logging/Logging.h>
#include <Shared/Types/Enum.h>

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

	bool LoadVisible = true;
	JsonHelpers::ParseBool(Data, "Visible", LoadVisible);
	if (!LoadVisible)
	{
		SetVisible(false);
	}

	bool LoadCastsShadow = true;
	JsonHelpers::ParseBool(Data, "CastShadow", LoadCastsShadow);
	if (!LoadCastsShadow)
	{
		SetCastShadow(false);
	}

	uint32_t CurrentSlot = 0;
	auto MaterialsIt = Data.Json.find("MaterialOverrides");
	if (MaterialsIt != Data.Json.end() && MaterialsIt->is_array())
	{
		for (const Json_t& MaterialNode : *MaterialsIt)
		{
			uint32_t Slot = CurrentSlot;
			JsonHelpers::ParseInt(MaterialNode, "Slot", Slot);

			if (OverrideMaterials.size() <= Slot)
			{
				OverrideMaterials.resize(Slot + 1, nullptr);
			}

			Path_s MaterialOverridePath;
			if (ENSUREMSG(JsonHelpers::ParsePath(MaterialNode, "MaterialAssetPath", MaterialOverridePath), "[MeshComponent_c::Deserialize] Failed to load override path"))
			{
				OverrideMaterials[Slot] = MaterialManager::RequestMaterialInstance(MaterialOverridePath);
			}

			CurrentSlot = static_cast<uint32_t>(OverrideMaterials.size());
		}
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
	if (!Visible || !Mesh)
		return;

	const matrix& WorldMatrix = GetWorldMatrix();
	const matrix& PrevWorldMatrix = MotionHistory.Update(WorldMatrix, Collector.FrameIndex, ConsumeMotionReset());

	ObjectUniforms_s Uniforms = {};
	const bool Mirrored = MakeObjectUniforms(WorldMatrix, PrevWorldMatrix, Uniforms);

	Mesh->Render(Collector, Collector.Alloc(Uniforms), Mirrored, OverrideMaterials);

	// TODO RT: For now we only use rays to cast shadows so we can disable shadow casting by removing from the RT scene
	// but in the future we should use instance masks instead in case we want reflections e.g
	if (EnumHasFlag(Collector.Flags, CollectorFlags_e::RAYTRACING_INSTANCES) && Mesh->RTGeom)
	{
		rl::RaytracingInstance& Instance = Collector.AddRaytracingInstance();
		Instance.Geometry = Mesh->RTGeom;
		// Row vectors, so the instance's column vector 3x4 is the transposed upper 4x3
		const matrix3x4 Transform = MakeMatrix3x4(TransposeMatrix(WorldMatrix));
		memcpy(Instance.Transform, Transform.m, sizeof(Instance.Transform));
		// TODO RT: mirrored instances need TRIANGLE_FRONT_COUNTERCLOCKWISE once rays are traced
	}

	if (EnumHasFlag(Collector.Flags, CollectorFlags_e::DISTANCE_FIELD_INSTANCES) && Mesh->SDF)
	{
		const SignedDistanceField_s& SDF = *Mesh->SDF;
		DistanceFieldInstance_s& DFInstance = Collector.AddDistanceFieldInstance();
		MakeDistanceFieldInstance(GetWorldMatrix(), SDF.VolumeBounds, rl::GetDescriptorIndex(SDF.TextureSRV), SDF.MaxDistance, DFInstance);
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

void MeshComponent_c::SetCastShadow(bool InCastShadow)
{
	if (CastShadow == InCastShadow)
		return;

	CastShadow = InCastShadow;

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

		bool LoadVisible = true;
		JsonHelpers::ParseBool(Data, "Visible", LoadVisible);
		if (!LoadVisible)
		{
			MeshComponent->SetVisible(false);
		}

		bool LoadCastsShadow = true;
		JsonHelpers::ParseBool(Data, "CastShadow", LoadCastsShadow);
		if (!LoadCastsShadow)
		{
			MeshComponent->SetCastShadow(false);
		}
	}
}