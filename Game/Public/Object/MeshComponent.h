#pragma once

#include "Rendering/IRenderable.h"
#include "Object/SpatialObject.h"
#include "Object/SpatialObjectComponent.h"

#include <string>

class MeshComponent_c : public SpatialObjectComponent_c, public IRenderable_c
{
public:

	using SpatialObjectComponent_c::SpatialObjectComponent_c;
	virtual ~MeshComponent_c() = default;

	// Begin ObjectComponent_c interface
	virtual void Deserialize(const struct JsonValue_s& Data) override;
	// End ObjectComponent_c interface

	// Begin IRenderable_c interface
	virtual void Render(struct SpatialRenderingCollector_s& Collector) override;
	// End IRenderable_c interface

	virtual void SetMesh(const std::shared_ptr<struct Mesh_s>& InMesh);

protected:

	std::shared_ptr<struct Mesh_s> Mesh = {};
};

class MeshObject_c : public SpatialObject_c
{
public:
	using SpatialObject_c::SpatialObject_c;
	virtual ~MeshObject_c() = default;

	// Begin Object_c interface
	virtual void Deserialize(const struct JsonValue_s& Data) override;
	virtual void OnCreate() override;
	// End Object_c interface

	MeshComponent_c* MeshComponent = nullptr;
};