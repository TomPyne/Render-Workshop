#include "Rendering/SpatialRenderingCollector.h"

FrameBufferAlloc_s SpatialRenderingCollector_s::Alloc(const void* Data, uint32_t Size)
{
	return FrameBuffer.Alloc(Data, Size);
}

FrameBufferAlloc_s SpatialRenderingCollector_s::AllocData(const void* Data, uint32_t Size)
{
	return FrameBuffer.AllocData(Data, Size);
}

void SpatialRenderingCollector_s::AddDebugLine(const float3& A, const float3& B, uint32_t Color, DebugDepthMode_e Mode)
{
	if (!IsDebugDrawEnabled())
		return;

	std::vector<DebugVertex_s>& Vertices = DebugVertices[static_cast<size_t>(DebugPrimitive_e::Lines)][static_cast<size_t>(Mode)];
	Vertices.push_back({ A, Color });
	Vertices.push_back({ B, Color });
}

void SpatialRenderingCollector_s::AddDebugTriangle(const float3& A, const float3& B, const float3& C, uint32_t Color, DebugDepthMode_e Mode)
{
	if (!IsDebugDrawEnabled())
		return;

	std::vector<DebugVertex_s>& Vertices = DebugVertices[static_cast<size_t>(DebugPrimitive_e::Triangles)][static_cast<size_t>(Mode)];
	Vertices.push_back({ A, Color });
	Vertices.push_back({ B, Color });
	Vertices.push_back({ C, Color });
}

uint32_t SpatialRenderingCollector_s::GetNumBatches() const
{
	uint32_t Num = 0;
	for (uint32_t PassIndex = 0; PassIndex < static_cast<uint32_t>(SpatialShaderPass_e::COUNT); ++PassIndex)
	{
		const SpatialRenderingMeshPass_s& Pass = Passes[PassIndex];
		Num += static_cast<uint32_t>(Pass.Batches.size());
	}
	return Num;
}
