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
public:
	using SpatialObjectComponent_c::SpatialObjectComponent_c;
	virtual ~RuntimeMeshComponent_c() = default;

	// IRenderable_c
	virtual void Render(struct SpatialRenderingCollector_s& Collector) override;
	// ~IRenderable_c

	void UpdateMesh(const RuntimeMeshDesc_s& Desc);

	std::shared_ptr<Mesh_s> Mesh = {};
};

class RuntimeMeshObject_c : public SpatialObject_c
{
public:
	using SpatialObject_c::SpatialObject_c;
	virtual ~RuntimeMeshObject_c() = default;

	// Begin SpatialObject_c
	virtual void OnConstruct() override;
	// End SpatialObject_c

	RuntimeMeshComponent_c* MeshComponent = nullptr;
};