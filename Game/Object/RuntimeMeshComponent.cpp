#include "RuntimeMeshComponent.h"

#include <Render/Render.h>

void RuntimeMeshComponent_c::UpdateMesh(const RuntimeMeshDesc_s& Desc)
{
	if (Desc.Positions && !Desc.Positions->empty())
	{
		PositionBuffer = rl::CreateVertexBufferFromArray(Desc.Positions->data(), Desc.Positions->size());
	}

	if (Desc.Indices && !Desc.Indices->empty())
	{
		IndexBuffer = rl::CreateIndexBufferFromArray(Desc.Indices->data(), Desc.Indices->size());
	}
}
