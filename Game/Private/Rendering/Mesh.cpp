#include "Rendering/Mesh.h"

#include "Rendering/SpaceRenderer.h"
#include "Rendering/Materials.h"
#include <Render/Render.h>
#include <Shared/Logging/Logging.h>

bool MakeObjectUniforms(const matrix& WorldMatrix, ObjectUniforms_s& OutUniforms)
{
	float Determinant = 0.0f;
	const matrix Inverse = InverseMatrix(WorldMatrix, &Determinant);

	// Normals are covectors, so they transform by the inverse transpose rather than the model matrix.
	// The determinant sign is kept separately: it flips tangent frame handedness under mirroring.
	const bool Mirrored = Determinant < 0.0f;

	OutUniforms.ModelMatrix = WorldMatrix;
	OutUniforms.NormalMatrix = TransposeMatrix(Inverse);
	OutUniforms.DeterminantSign = Mirrored ? -1.0f : 1.0f;

	return Mirrored;
}

void Mesh_s::Render(SpatialRenderingCollector_s& Collector, rl::DynamicBuffer_t DynamicUniforms, bool Mirrored) const
{
	if (!Ready)
		return;

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

			Batch.PSO = Surface.Material->GetPSO(Mirrored);
			Batch.MaterialUniforms = Surface.Material->GetConstantBuffer();
		}
	}
}
