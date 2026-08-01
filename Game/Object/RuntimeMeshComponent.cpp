#include "RuntimeMeshComponent.h"

#include "Game/Rendering/SpaceRenderer.h"
#include "Game/Rendering/Materials.h"
#include <Render/Render.h>

RuntimeMeshComponent_c::RuntimeMeshComponent_c(const std::shared_ptr<SpatialObject_c>& InSpatialOwner)
	: SpatialObjectComponent_c(InSpatialOwner)
{}

void RuntimeMeshComponent_c::Render(SpatialRenderingCollector_s& Collector)
{
	rl::DynamicBuffer_t DynamicUniforms = rl::CreateDynamicConstantBuffer(&GetTransform().GetMatrix());
	Mesh.Render(Collector, DynamicUniforms);
}

void RuntimeMeshComponent_c::UpdateMesh(const RuntimeMeshDesc_s& Desc)
{
	if (!Desc.Positions || Desc.Positions->empty())
	{
		return;
	}

	if (!Desc.Indices || Desc.Indices->empty())
	{
		return;
	}

	Mesh.PositionBuffer = rl::CreateStructuredBuffer(Desc.Positions->data(), Desc.Positions->size());
	Mesh.PositionBufferSRV = rl::CreateStructuredBufferSRV(Mesh.PositionBuffer, 0, static_cast<uint32_t>(Desc.Positions->size()), static_cast<uint32_t>(sizeof(float3)));

	Mesh.IndexBuffer = rl::CreateIndexBufferFromArray(Desc.Indices->data(), Desc.Indices->size());

	Mesh.Surfaces.push_back({});
	Surface_s& Surface = Mesh.Surfaces.back();
	Surface.Material = nullptr;
	Surface.IndexOffset = 0;
	Surface.IndexCount = static_cast<uint32_t>(Desc.Indices->size());

	MeshUniformData_s MeshData = {};
	MeshData.PositionBufferIndex = rl::GetDescriptorIndex(Mesh.PositionBufferSRV);
	Mesh.MeshUniforms = rl::CreateConstantBuffer(&MeshData);
}

void RuntimeMeshComponent_c::SetMaterial(BasicMaterial_c* InMaterial)
{
	if (!Mesh.Surfaces.empty())
	{
		Mesh.Surfaces[0].Material = InMaterial;

	}
}

void RuntimeMeshObject_c::OnCreate()
{
	MeshComponent = AddSpatialComponent<RuntimeMeshComponent_c>();
}
