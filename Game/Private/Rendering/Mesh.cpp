#include "Rendering/Mesh.h"

#include "Assets/AssetManager.h"
#include "Rendering/SpaceRenderer.h"
#include "Rendering/Materials.h"
#include <Render/Render.h>
#include <Shared/Logging/Logging.h>

bool MakeObjectUniforms(const matrix& WorldMatrix, const matrix& PrevWorldMatrix, ObjectUniforms_s& OutUniforms)
{
	float Determinant = 0.0f;
	const matrix Inverse = InverseMatrix(WorldMatrix, &Determinant);

	// Normals are covectors, so they transform by the inverse transpose rather than the model matrix.
	// The determinant sign is kept separately: it flips tangent frame handedness under mirroring.
	const bool Mirrored = Determinant < 0.0f;

	OutUniforms.ModelMatrix = WorldMatrix;
	OutUniforms.NormalMatrix = TransposeMatrix(Inverse);
	OutUniforms.PrevModelMatrix = PrevWorldMatrix;
	OutUniforms.DeterminantSign = Mirrored ? -1.0f : 1.0f;

	return Mirrored;
}

const matrix& ObjectMotionHistory_s::Update(const matrix& WorldMatrix, uint64_t FrameIndex, bool Reset)
{
	// A second draw in the same frame (another view) must not advance the history
	if (FrameIndex != LastFrameIndex)
	{
		const bool DrawnLastFrame = LastFrameIndex != 0 && LastFrameIndex + 1 == FrameIndex;
		PrevWorldMatrix = (DrawnLastFrame && !Reset) ? CurrWorldMatrix : WorldMatrix;
		CurrWorldMatrix = WorldMatrix;
		LastFrameIndex = FrameIndex;
	}
	else if (Reset)
	{
		PrevWorldMatrix = WorldMatrix;
	}

	return PrevWorldMatrix;
}

void Mesh_s::Render(SpatialRenderingCollector_s& Collector, FrameBufferAlloc_s DynamicUniforms, bool Mirrored, const std::vector<std::shared_ptr<MaterialShaderInstance_c>>& MaterialOverrides) const
{
	if (!Ready)
		return;
	for (uint32_t SurfaceIt = 0; SurfaceIt < Surfaces.size(); SurfaceIt++)
	{
		const Surface_s& Surface = Surfaces[SurfaceIt];

		MaterialShaderInstance_c* Material = Surface.Material.get();
		if (MaterialOverrides.size() > SurfaceIt && MaterialOverrides[SurfaceIt] != nullptr)
		{
			Material = MaterialOverrides[SurfaceIt].get();
		}
		
		if (Material)
		{
			if (!Material->IsReady())
			{
				Material = AssetManager_c::GetDefaultMaterial().get();
				if (!Material)
					continue;
			}

			rl::GraphicsPipelineState_t PSO = Material->GetPSO(Mirrored);
			rl::ConstantBuffer_t MaterialConstants = Material->GetConstantBuffer();

			if (!rl::IsValid(PSO) || !rl::IsValid(MaterialConstants))
				continue;

			SpatialRenderingBatch_s& Batch = Collector.MainPass.AddBatch();
			
			Batch.IndexBuffer = IndexBuffer;
			Batch.IndexBufferFormat = rl::RenderFormat::R32_UINT;
			Batch.IndexCount = Surface.IndexCount;
			Batch.IndexOffset = Surface.IndexOffset;

			Batch.DynamicUniforms = DynamicUniforms;
			Batch.MeshUniforms = MeshUniforms;

			Batch.PSO = PSO;
			Batch.MaterialUniforms = MaterialConstants;
		}
	}
}
