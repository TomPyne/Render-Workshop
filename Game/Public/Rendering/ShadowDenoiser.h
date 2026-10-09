#pragma once

#include <Render/RenderTypes.h>
#include <RenderUtils/RenderGraph/RenderGraph.h>
#include <SurfMath.h>

#include <cstdint>

enum class ShadowVisualise_e : uint32_t
{
	Shadow,
	Variance,
	Confidence,
};

enum class ShadowHistoryFormat_e : uint32_t
{
	RGBA16Float,
	RGB10A2Unorm,
};

struct ShadowDenoiseSettings_s
{
	// Off passes the raw shadow through. History is still written, with zero confidence, so re-enabling needs no reset.
	bool TemporalEnabled = true;
	// History weight approaches MaxConfidence at ConfidenceRate per frame while reprojection succeeds.
	// Higher values are less noisy but ghost longer behind moving shadow casters.
	float MaxConfidence = 0.9f;
	float ConfidenceRate = 0.1f;
	// History is rejected when its view depth differs from the expected depth by more than this fraction
	float DepthTolerance = 0.02f;
	// Below this confidence, variance comes from the current frame's neighbourhood instead of the history moments
	float MinConfidenceForTemporalVariance = 0.5f;
	// Unorm halves the history memory. Its steps are 1/1023 everywhere, coarser than float's near 1, so a slow blend can stall short of converging.
	ShadowHistoryFormat_e HistoryFormat = ShadowHistoryFormat_e::RGB10A2Unorm;

	// Only applies in the Lighting view mode
	ShadowVisualise_e Visualise = ShadowVisualise_e::Shadow;
};

struct ShadowDenoiseOutputs_s
{
	// .r is the shadow, .g its variance
	RenderGraphResourceHandle_t Shadow = RenderGraphResourceHandle_t::NONE;
	// Shadow mean, second moment, confidence
	RenderGraphResourceHandle_t History = RenderGraphResourceHandle_t::NONE;
};

// Denoises the raytraced shadow mask, keeping history across frames
class ShadowDenoiser_c
{
public:
	void Init();

	// Call whenever last frame's view can no longer be reprojected into
	void ResetHistory();

	ShadowDenoiseOutputs_s AddPasses(RenderGraphBuilder_s& RGBuilder, const ShadowDenoiseSettings_s& Settings, RenderGraphResourceHandle_t RawShadow,
		RenderGraphResourceHandle_t SceneDepth, RenderGraphResourceHandle_t SceneVelocity, RenderGraphResourceHandle_t SceneNormalRoughness,
		const matrix& InvViewProjection, uint2 Size);

	// Call after the graph has executed, this frame's output becomes next frame's history
	void EndFrame();

	// Both ping-pong textures, 0 before the first AddPasses
	uint64_t GetHistoryMemoryBytes() const;
	uint64_t GetLinearDepthHistoryMemoryBytes() const;

private:
	rl::ComputePipelineStatePtr TemporalPSO = {};

	// Ping-ponged, one is read as history while the other is written
	RenderGraphTexturePtr_t HistoryTextures[2] = {}; // Shadow mean, second moment, confidence
	RenderGraphTexturePtr_t LinearDepthHistoryTextures[2] = {};
	uint2 HistorySize = { 0u, 0u };
	// What HistoryTextures were created with
	ShadowHistoryFormat_e HistoryFormat = ShadowHistoryFormat_e::RGBA16Float;
	uint32_t HistoryReadIndex = 0;
	bool HistoryValid = false;
};
