#pragma once

#include "Object/SpatialObject.h"
#include "Object/SpatialObjectComponent.h"
#include "Physics/IPhysical.h"
#include "Rendering/IRenderable.h"

class MeshComponent_c : public SpatialObjectComponent_c, public IRenderable_c, public IPhysical_c
{
	OBJECTCOMPONENT_BODY(MeshComponent_c, SpatialObjectComponent_c)

	// Begin ObjectComponent_c interface
	virtual void Deserialize(const struct JsonValue_s& Data) override;
	// End ObjectComponent_c interface

	// Begin IRenderable_c interface
	virtual void Render(struct SpatialRenderingCollector_s& Collector) override;
	// End IRenderable_c interface

	// Begin IPhysical interface
	virtual void Intersect(IntersectionCtx_s& Context) const override;
	// End IPhysical interface

	virtual void SetMesh(const std::shared_ptr<struct Mesh_s>& InMesh);
	void SetVisible(bool InVisible) { Visible = InVisible; }
	void SetCollidable(bool InCollidable) { Collidable = InCollidable; }

	uint32_t GetMaterialCount() const;
	class MaterialShaderInstance_c* GetMaterial(uint32_t Index) const;

protected:

	std::shared_ptr<struct Mesh_s> Mesh = {};
	bool Visible = true;
	bool Collidable = true;
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