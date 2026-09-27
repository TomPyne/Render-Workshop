#include "RaytracingBuildPass.h"

#include <Logging/Logging.h>
#include <RenderUtils/GPUContext/GPUContext.h>

#include <vector>

void AddRaytracingBuildPass(RenderGraphBuilder_s& RGBuilder, RenderGraphResourceHandle_t Scene, std::span<const rl::RaytracingGeometry_t> Geometries, std::span<const rl::RaytracingInstance> Instances)
{
	const uint32_t InstanceCount = static_cast<uint32_t>(Instances.size());

	const RenderGraphResourceDesc_s& SceneDesc = RGBuilder.GetResourceDesc(Scene);
	if (!ENSUREMSG(SceneDesc.Kind == RenderGraphResourceKind_e::RAYTRACING_SCENE, "AddRaytracingBuildPass needs a handle from ImportRaytracingScene"))
		return;

	if (!ENSUREMSG(InstanceCount * rl::RaytracingInstanceDescSize <= FrameBuffer_s::MaxAllocSize, "Too many raytracing instances for one FrameBuffer alloc"))
		return;

	const rl::RaytracingScene_t RTScene = SceneDesc.RaytracingScene;

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
	.SetExecuteCallback([PreparedGeometries = std::move(PreparedGeometries), RTScene, InstanceAlloc, InstanceCount](RenderGraph_s& RG, GPUContext_s& Ctx)
	{
		if (!PreparedGeometries.empty())
		{
			Ctx.BuildRaytracingGeometry(PreparedGeometries.data(), static_cast<uint32_t>(PreparedGeometries.size()));
		}

		Ctx.BuildRaytracingScene(RTScene, InstanceAlloc, InstanceCount);
	});
}
