#pragma once

#include <Render/Raytracing.h>
#include <RenderUtils/RenderGraph/RenderGraph.h>

#include <span>

struct RaytracingSceneFrameData_s
{
	std::vector<rl::RaytracingGeometry_t> Geometries;
	bool RaytracingBuildRequired = false;
	RenderGraphResourceHandle_t RTSceneHandle;
};

struct RaytracingScene_s
{	
	void Init();
	RaytracingSceneFrameData_s PrepareFrame(bool SceneDirty);
	void AddRaytracingBuildPass(RenderGraphBuilder_s& RGBuilder, const RaytracingSceneFrameData_s& FrameData, std::span<const rl::RaytracingInstance> Instances);

	RenderGraphResourceHandle_t GetRTSceneHandle(RenderGraphBuilder_s& RGBuilder) const;
private:
	rl::RaytracingScenePtr RTScene = {}; // Invalid without raytracing support
};