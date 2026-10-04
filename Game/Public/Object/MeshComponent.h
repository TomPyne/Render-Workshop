#pragma once

#include "Object/SpatialObject.h"
#include "Object/SpatialObjectComponent.h"
#include "Physics/IPhysical.h"
#include "Rendering/IRenderable.h"
#include "Rendering/Mesh.h"

#include <vector>

class MaterialShaderInstance_c;

class MeshComponent_c : public SpatialObjectComponent_c, public IRenderable_c, public IPhysical_c
{
	OBJECTCOMPONENT_BODY(MeshComponent_c, SpatialObjectComponent_c)

	// Begin ObjectComponent_c interface
	virtual void OnCreate() override;
	virtual void Deserialize(const struct JsonValue_s& Data) override;
	virtual void PreDestroy() override;
	// End ObjectComponent_c interface

	// Begin SpatialObjectComponent_c interface
	virtual void OnTransformed() override;
	// End SpatialObjectComponent_c interface

	// Begin IRenderable_c interface
	virtual void Render(struct SpatialRenderingCollector_s& Collector) override;
	virtual void CollectRaytracingInstances(std::vector<rl::RaytracingInstance>& OutInstances) override;
	virtual void CollectDistanceFieldInstances(std::vector<DistanceFieldInstance_s>& OutInstances) override;
	// End IRenderable_c interface

	// Begin IPhysical interface
	virtual void Intersect(IntersectionCtx_s& Context) const override;
	// End IPhysical interface

	virtual void SetMesh(const std::shared_ptr<struct Mesh_s>& InMesh);
	void SetVisible(bool InVisible);
	void SetCollidable(bool InCollidable) { Collidable = InCollidable; }
	void SetCastShadow(bool InCastShadow);

	uint32_t GetMaterialCount() const;
	class MaterialShaderInstance_c* GetMaterial(uint32_t Index) const;

protected:

	std::shared_ptr<struct Mesh_s> Mesh = {};
	bool Visible = true;
	bool Collidable = true;
	bool CastShadow = true;

	std::vector<std::shared_ptr<MaterialShaderInstance_c>> OverrideMaterials;

	ObjectMotionHistory_s MotionHistory;
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