#pragma once

#include <Render/Raytracing.h>

#include <vector>

struct DistanceFieldInstance_s;

class IRenderable_c
{
public:

	virtual ~IRenderable_c() = default;

	virtual void Render(struct SpatialRenderingCollector_s& Collector) = 0;

	// Walked only when the raytracing scene is rebuilt
	virtual void CollectRaytracingInstances(std::vector<rl::RaytracingInstance>& OutInstances) {}

	// Walked every frame to build the distance field scene
	virtual void CollectDistanceFieldInstances(std::vector<DistanceFieldInstance_s>& OutInstances) {}

};