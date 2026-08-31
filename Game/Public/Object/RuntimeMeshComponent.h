#pragma once


#include "Rendering/IRenderable.h"
#include "Rendering/Mesh.h"
#include "Object/SpatialObject.h"
#include "Object/SpatialObjectComponent.h"

#include <Render/RenderTypes.h>

#include <SurfMath.h>

#include <memory>
#include <vector>

struct RuntimeMeshDesc_s
{
	std::vector<float3>* Positions;
	std::vector<uint32_t>* Indices;
};

class RuntimeMeshComponent_c : public SpatialObjectComponent_c, public IRenderable_c
{
	OBJECTCOMPONENT_BODY(RuntimeMeshComponent_c, SpatialObjectComponent_c)

	// IRenderable_c
	virtual void Render(struct SpatialRenderingCollector_s& Collector) override;
	// ~IRenderable_c

	void UpdateMesh(const RuntimeMeshDesc_s& Desc);

	std::shared_ptr<Mesh_s> Mesh = {};
};

class RuntimeMeshObject_c : public SpatialObject_c
{
	OBJECT_BODY(RuntimeMeshObject_c, SpatialObject_c)

	// Begin SpatialObject_c
	virtual void OnConstruct() override;
	// End SpatialObject_c

	RuntimeMeshComponent_c* MeshComponent = nullptr;
};