#pragma once

#include "Rendering/IRenderable.h"
#include "Object/SpatialObject.h"
#include "Object/SpatialObjectComponent.h"

#include <string>

class MeshComponent_c : public SpatialObjectComponent_c, public IRenderable_c
{
	OBJECTCOMPONENT_BODY(MeshComponent_c, SpatialObjectComponent_c)

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
	OBJECT_BODY(MeshObject_c, SpatialObject_c)

	// Begin Object_c interface
	virtual void OnConstruct() override;
	virtual void Deserialize(const struct JsonValue_s& Data) override;
	// End Object_c interface

	MeshComponent_c* MeshComponent = nullptr;
};