#include "RaytracingScene.h"

#include "Assets/AssetManager.h"
#include "Rendering/Mesh.h"

#include <RenderUtils/GPUContext/GPUContext.h>
#include <Shared/Logging/Logging.h>

void RaytracingScene_s::Init()
{
	if (rl::Render_SupportsRaytracing())
	{
		RTScene = rl::CreateRaytracingScene();
	}
	else
	{
		LOGINFO("[RaytracingScene_s::Init] Raytracing not supported, acceleration structures will not be built");
	}
}

RaytracingSceneFrameData_s RaytracingScene_s::PrepareFrame(bool SceneDirty)
{
	RaytracingSceneFrameData_s FrameData = {};
	if (!RTScene)
		return FrameData;

	std::vector<const Mesh_s*> MeshesToBuild = AssetManager_c::Get().CollectMeshesForRTBuild();

	FrameData.Geometries.reserve(MeshesToBuild.size());
	for (const Mesh_s* Mesh : MeshesToBuild)
	{
		if (Mesh->RTGeom)
		{
			FrameData.Geometries.push_back(Mesh->RTGeom);
		}
	}

	// New geometry has no instances in the current scene, so it always needs a rebuild
	FrameData.RaytracingBuildRequired = !FrameData.Geometries.empty() || SceneDirty;

	return FrameData;
}

void RaytracingScene_s::AddRaytracingBuildPass(RenderGraphBuilder_s& RGBuilder, const RaytracingSceneFrameData_s& FrameData, std::span<const rl::RaytracingInstance> Instances)
{
	if (!FrameData.RaytracingBuildRequired)
		return;

	std::span<const rl::RaytracingGeometry_t> Geometries = FrameData.Geometries;
	const RenderGraphResourceHandle_t Scene = GetRTSceneHandle(RGBuilder);

	const uint32_t InstanceCount = static_cast<uint32_t>(Instances.size());

	const RenderGraphResourceDesc_s& SceneDesc = RGBuilder.GetResourceDesc(Scene);
	if (!ENSUREMSG(SceneDesc.Kind == RenderGraphResourceKind_e::RAYTRACING_SCENE, "AddRaytracingBuildPass needs a handle from ImportRaytracingScene"))
		return;

	if (!ENSUREMSG(InstanceCount * rl::RaytracingInstanceDescSize <= FrameBuffer_s::MaxAllocSize, "Too many raytracing instances for one FrameBuffer alloc"))
		return;

	const rl::RaytracingScene_t RTSceneHandle = RTScene.Get();

	if (!rl::PrepareRaytracingSceneBuild(RTScene, InstanceCount))
	{
		LOGERROR("Failed to prepare raytracing scene build %S", SceneDesc.ResourceName.c_str());
		return;
	}

	std::vector<rl::RaytracingGeometry_t> PreparedGeometries;
	PreparedGeometries.reserve(Geometries.size());
	for (rl::RaytracingGeometry_t Geometry : Geometries)
	{
		if (rl::PrepareRaytracingGeometryBuild(Geometry))
		{
			PreparedGeometries.push_back(Geometry);
		}
	}

	FrameBufferAlloc_s InstanceAlloc = {};
	void* InstanceData = InstanceCount > 0u ? RGBuilder.GetMainFrameBuffer().AllocRaw(InstanceCount * rl::RaytracingInstanceDescSize, InstanceAlloc) : nullptr;
	rl::WriteRaytracingInstances(RTScene, InstanceData, Instances.data(), InstanceCount);

	RGBuilder.AddPass(RenderGraphPassType_e::RAYTRACING, L"Raytracing Scene Build")
	.AccessResource(Scene, RenderGraphResourceAccessType_e::UAV, RenderGraphLoadOp_e::DONT_CARE)
	.SetExecuteCallback([PreparedGeometries = std::move(PreparedGeometries), RTSceneHandle, InstanceAlloc, InstanceCount](RenderGraph_s& RG, GPUContext_s& Ctx)
	{
		if (!PreparedGeometries.empty())
		{
			Ctx.BuildRaytracingGeometry(PreparedGeometries.data(), static_cast<uint32_t>(PreparedGeometries.size()));
		}

		Ctx.BuildRaytracingScene(RTSceneHandle, InstanceAlloc, InstanceCount);
	});
}

RenderGraphResourceHandle_t RaytracingScene_s::GetRTSceneHandle(RenderGraphBuilder_s& RGBuilder) const
{
	return RGBuilder.ImportRaytracingScene(RTScene, L"RaytracingScene");
}
