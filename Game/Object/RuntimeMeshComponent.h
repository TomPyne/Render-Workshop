#pragma once

#include "SpatialObjectComponent.h"
#include "Game/Rendering/IRenderable.h"
#include "SpatialObject.h"

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
	RuntimeMeshComponent_c(const std::shared_ptr<SpatialObject_c>& InSpatialOwner);
	virtual ~RuntimeMeshComponent_c() = default;

	// IRenderable_c
	virtual void Render(struct SpatialRenderingCollector_s& Collector) override;
	// ~IRenderable_c

	void UpdateMesh(const RuntimeMeshDesc_s& Desc);
	void SetMaterial(class BasicMaterial_c* InMaterial) { Material = InMaterial; }

	rl::StructuredBufferPtr PositionBuffer = {};
	rl::ShaderResourceViewPtr PositionBufferSRV = {};
	rl::IndexBufferPtr IndexBuffer = {};
	uint32_t IndexCount = 0;
	rl::ConstantBuffer_t MeshUniforms = {};

	class BasicMaterial_c* Material = nullptr;
};

class RuntimeMeshObject_c : public SpatialObject_c
{
public:
	using SpatialObject_c::SpatialObject_c;

	virtual void OnCreate() override;

	RuntimeMeshComponent_c* MeshComponent = nullptr;
};