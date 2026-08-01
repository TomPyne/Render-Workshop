#include "Mesh.h"

#include "Game/Rendering/SpaceRenderer.h"
#include "Game/Rendering/Materials.h"
#include <Render/Render.h>

void Mesh_s::Render(SpatialRenderingCollector_s& Collector, rl::DynamicBuffer_t DynamicUniforms) const
{
	for (const Surface_s& Surface : Surfaces)
	{
		if (Surface.Material)
		{
			SpatialRenderingBatch_s& Batch = Collector.MainPass.AddBatch();
			
			Batch.IndexBuffer = IndexBuffer;
			Batch.IndexBufferFormat = rl::RenderFormat::R32_UINT;
			Batch.IndexCount = Surface.IndexCount;
			Batch.IndexOffset = Surface.IndexOffset;

			Batch.DynamicUniforms = DynamicUniforms;
			Batch.MeshUniforms = MeshUniforms;

			Batch.PSO = Surface.Material->PSO;
			Batch.MaterialUniforms = Surface.Material->MaterialConstants;
		}
	}
}
