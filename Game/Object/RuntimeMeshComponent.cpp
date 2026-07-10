#include "RuntimeMeshComponent.h"

#include "Game/Rendering/SpaceRenderer.h"
#include "Game/Rendering/Materials.h"
#include <Render/Render.h>

struct MeshUniforms_s
{
	uint32_t PositionBufferIndex;
	float __pad[3];
};

RuntimeMeshComponent_c::RuntimeMeshComponent_c(const std::shared_ptr<SpatialObject_c>& InSpatialOwner)
	: SpatialObjectComponent_c(InSpatialOwner)
{}

void RuntimeMeshComponent_c::Render(SpatialRenderingCollector_s& Collector)
{
	if (Material)
	{
		SpatialRenderingBatch_s& Batch = Collector.MainPass.AddBatch();
		Batch.PSO = Material->PSO;
		Batch.IndexBuffer = IndexBuffer;
		Batch.MeshUniforms = MeshUniforms;
		Batch.MaterialUniforms = Material->MaterialConstants;
	}

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

	PositionBuffer = rl::CreateStructuredBuffer(Desc.Positions->data(), Desc.Positions->size());
	PositionBufferSRV = rl::CreateStructuredBufferSRV(PositionBuffer, 0, static_cast<uint32_t>(Desc.Positions->size()), static_cast<uint32_t>(sizeof(float3)));

	IndexBuffer = rl::CreateIndexBufferFromArray(Desc.Indices->data(), Desc.Indices->size());

	MeshUniforms_s MeshData = {};
	MeshData.PositionBufferIndex = rl::GetDescriptorIndex(PositionBufferSRV);
	MeshUniforms = rl::CreateConstantBuffer(&MeshData);
}

RuntimeMeshObject_c::RuntimeMeshObject_c(const ObjectArgs_s& Args)
	: SpatialObject_c(Args)
{
	MeshComponent = AddSpatialComponent<RuntimeMeshComponent_c>();
}
