#pragma once

#include <Render/Raytracing.h>
#include <RenderUtils/RenderGraph/RenderGraph.h>

#include <span>

// Prepares the geometry builds and a full scene build. Geometries contains all the geometry that needs a build this frame.
// Scene is always rebuilt when calling this, so Scene must be passed and full scenes Instances must be provided each time.
void AddRaytracingBuildPass(RenderGraphBuilder_s& RGBuilder, RenderGraphResourceHandle_t Scene, std::span<const rl::RaytracingGeometry_t> Geometries, std::span<const rl::RaytracingInstance> Instances);
