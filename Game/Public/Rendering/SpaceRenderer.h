#pragma once

#include "Rendering/DebugViewPass.h"
#include "Rendering/DistanceFieldAOPass.h"
#include "Rendering/DistanceFieldScene.h"
#include "Rendering/DistanceFieldVisualisePass.h"
#include "Rendering/GlobalDistanceField.h"
#include "Rendering/ShadowDenoiser.h"

#include <Render/RenderTypes.h>
#include <RenderUtils/RenderGraph/RenderGraph.h>
#include <SurfClock.h>

#include <unordered_map>
#include <vector>

namespace SpaceRendererRootSigSlots
{
	enum Value
	{
		RS_DRAWCONSTANTS,
		RS_VIEW_BUF,
		RS_MODEL_BUF,
		RS_MAT_BUF,
		RS_TLAS,
		RS_SRV_TABLE,
		RS_UAV_TABLE,
		RS_COUNT,
	};
}

namespace SpaceRendererCBVRegister
{
	enum Value
	{
		CBV_DRAWCONSTANTS = 0,
		CBV_VIEW_BUF = 1,
		CBV_MODEL_BUF = 2,
		CBV_MAT_BUF = 3,
	};
}

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

	ShadowDenoiseSettings_s ShadowDenoise;

	DebugViewMode_e DebugViewMode = DebugViewMode_e::Lit;

	const DistanceFieldScene_c& GetDistanceFieldScene() const { return DistanceFieldScene; }
	GlobalDistanceField_c& GetGlobalDistanceField() { return GlobalDistanceField; }
	ShadowDenoiser_c& GetShadowDenoiser() { return ShadowDenoiser; }

	DistanceFieldVisualiseSettings_s DistanceFieldVisualise;
	DistanceFieldAOSettings_s DistanceFieldAO;

	static rl::RootSignature_t GetRootSignature();
	static const rl::GraphicsPipelineTargetDesc& GetMaterialPipelineTargetDesc();
	static const rl::RenderFormat GetMaterialPipelineDepthFormat();

protected:

	RenderGraphResourcePool_s RenderGraphResourcePool;

	SurfClock Clock;

	uint64_t FrameIndex = 0;

	matrix PrevViewProjection;
	bool HasPrevView = false;

	ShadowDenoiser_c ShadowDenoiser;

	rl::RaytracingScenePtr RTScene = {}; // Invalid without raytracing support

	DistanceFieldScene_c DistanceFieldScene;
	GlobalDistanceField_c GlobalDistanceField;
	std::vector<DistanceFieldInstance_s> DistanceFieldInstances;

	std::shared_ptr<struct Texture_s> BlueNoiseTexture;
};