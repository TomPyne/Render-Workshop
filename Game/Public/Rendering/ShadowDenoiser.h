#pragma once

#include <Render/RenderTypes.h>
#include <RenderUtils/RenderGraph/RenderGraph.h>
#include <SurfMath.h>

#include <cstdint>

struct ShadowDenoiseSettings_s
{
	// History weight approaches MaxConfidence at ConfidenceRate per frame while reprojection succeeds.
	// Higher values are less noisy but ghost longer behind moving shadow casters.
	float MaxConfidence = 0.9f;
	float ConfidenceRate = 0.1f;
	// History is rejected when its view depth differs from the expected depth by more than this fraction
	float DepthTolerance = 0.02f;
};

// Denoises the raytraced shadow mask, keeping history across frames
class ShadowDenoiser_c
{
public:
	void Init();

	// Call whenever last frame's view can no longer be reprojected into
	void ResetHistory();

	// Returns the denoised shadow texture, .r is the shadow
	RenderGraphResourceHandle_t AddPasses(RenderGraphBuilder_s& RGBuilder, const ShadowDenoiseSettings_s& Settings, RenderGraphResourceHandle_t RawShadow,
		RenderGraphResourceHandle_t SceneDepth, RenderGraphResourceHandle_t SceneVelocity, const matrix& InvViewProjection, uint2 Size);

	// Call after the graph has executed, this frame's output becomes next frame's history
	void EndFrame();

private:
	rl::ComputePipelineStatePtr TemporalPSO = {};

	// Ping-ponged, one is read as history while the other is written
	RenderGraphTexturePtr_t HistoryTextures[2] = {}; // Shadow + confidence
	RenderGraphTexturePtr_t LinearDepthHistoryTextures[2] = {};
	uint2 HistorySize = { 0u, 0u };
	uint32_t HistoryReadIndex = 0;
	bool HistoryValid = false;
};
