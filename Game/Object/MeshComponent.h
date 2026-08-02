#pragma once

#include "Game/Rendering/IRenderable.h"
#include "SpatialObjectComponent.h"

#include <string>

class MeshComponent_c : public SpatialObjectComponent_c, public IRenderable_c
{
public:

	using SpatialObjectComponent_c::SpatialObjectComponent_c;
	virtual ~MeshComponent_c() = default;

	// Begin IRenderable_c interface
	virtual void Render(struct SpatialRenderingCollector_s& Collector) override;
	// End IRenderable_c interface

	virtual void SetMesh(const std::shared_ptr<struct Mesh_s>& InMesh);

protected:

	std::shared_ptr<struct Mesh_s> Mesh = {};
};