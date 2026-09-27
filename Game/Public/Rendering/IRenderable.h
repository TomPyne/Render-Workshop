#pragma once

#include <Render/Raytracing.h>

#include <vector>

class IRenderable_c
{
public:

	virtual ~IRenderable_c() = default;

	virtual void Render(struct SpatialRenderingCollector_s& Collector) = 0;

	// Walked only when the raytracing scene is rebuilt
	virtual void CollectRaytracingInstances(std::vector<rl::RaytracingInstance>& OutInstances) {}

};