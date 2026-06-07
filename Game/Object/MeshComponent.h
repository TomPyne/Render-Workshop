#pragma once

#include "Game/Rendering/IRenderable.h"
#include "SpatialObjectComponent.h"

class MeshComponent_c : public SpatialObjectComponent_c, public IRenderable_c
{
public:

	virtual ~MeshComponent_c() = default;

	// Begin IRenderable_c interface
	virtual void Render(struct SpatialRenderingCollector_s& Collector) override;
	// End IRenderable_c interface
};