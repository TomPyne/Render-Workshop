#pragma once

#include <Render/RenderTypes.h>
#include <RenderUtils/RenderGraph/RenderGraph.h>
#include <SurfClock.h>

#include <unordered_map>
#include <vector>

struct SpatialRenderingBatch_s
{
	FrameBufferAlloc_s DynamicUniforms;
	rl::ConstantBuffer_t MeshUniforms;
	rl::ConstantBuffer_t MaterialUniforms;
	rl::IndexBuffer_t IndexBuffer;
	rl::RenderFormat IndexBufferFormat;
	uint32_t IndexOffset;
	uint32_t IndexCount;
	rl::GraphicsPipelineState_t PSO;
};

struct SpatialRenderingMeshPass_s
{
	std::vector<SpatialRenderingBatch_s> Batches;

	static constexpr size_t ArenaSize = 100;

	SpatialRenderingBatch_s& AddBatch()
	{
		if ((Batches.size() % ArenaSize) == 0)
		{
			Batches.reserve(Batches.size() + ArenaSize);
		}
		Batches.resize(Batches.size() + 1);
		return Batches.back();
	}
};

enum class SpatialShader_t : uint32_t
{
	INVALID = 0,
};

enum class SpatialShaderPass_t : uint32_t
{
	INVALID = 0,
};

struct SpatialRenderingCollector_s
{
	SpatialRenderingCollector_s(FrameBuffer_s& InFrameBuffer, uint64_t InFrameIndex)
		: FrameIndex(InFrameIndex)
		, FrameBuffer(InFrameBuffer)
	{}

	template<typename T>
	FrameBufferAlloc_s Alloc(const T& Data)
	{
		static_assert(std::is_trivially_copyable_v<T>, "FrameBuffer data must be trivially copyable");
		return Alloc(&Data, sizeof(T));
	}

	FrameBufferAlloc_s Alloc(const void* Data, uint32_t Size);

	SpatialRenderingMeshPass_s MainPass;

	// Lets renderables detect whether they were drawn on the previous frame
	const uint64_t FrameIndex;

private:
	FrameBuffer_s& FrameBuffer;
};

struct SpaceRendererScreenInfo_s
{
	uint32_t Width;
	uint32_t Height;
	rl::RenderView* RenderView = nullptr;
};

class SpaceRenderer_c
{
public:
	void Init();
	void RenderSpace(const SpaceRendererScreenInfo_s& Screen, class Space_c* Space, rl::CommandListSubmissionGroup& clGroup);

	// Call whenever last frame's view can no longer be reprojected into, e.g. resize or camera change
	void ResetTemporalHistory();

	static rl::RootSignature_t GetRootSignature();
	static const rl::GraphicsPipelineTargetDesc& GetMaterialPipelineTargetDesc();
protected:

	RenderGraphResourcePool_s RenderGraphResourcePool;

	SurfClock Clock;

	uint64_t FrameIndex = 0;

	matrix PrevViewProjection;
	bool HasPrevView = false;

	rl::RaytracingScenePtr RTScene = {}; // Invalid without raytracing support

	std::shared_ptr<struct Texture_s> BlueNoiseTexture;
};