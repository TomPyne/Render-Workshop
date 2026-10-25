#pragma once

#include "Rendering/DebugViewPass.h"
#include "Rendering/DistanceFieldAOPass.h"
#include "Rendering/DistanceFieldScene.h"
#include "Rendering/DistanceFieldVisualisePass.h"
#include "Rendering/GlobalDistanceField.h"
#include "Rendering/ShadowDenoiser.h"
#include "Rendering/Skylight.h"
#include "Rendering/SpatialRenderingCollector.h"

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
		CBV_VIEW_BUF = 1,
		CBV_MODEL_BUF = 2,
		CBV_MAT_BUF = 3,
	};
}

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

	// Gates debug lines and shapes submitted through the collector
	bool DebugDraw = true;

	const DistanceFieldScene_c& GetDistanceFieldScene() const { return DistanceFieldScene; }
	GlobalDistanceField_c& GetGlobalDistanceField() { return GlobalDistanceField; }
	ShadowDenoiser_c& GetShadowDenoiser() { return ShadowDenoiser; }
	SkylightRenderer_c& GetSkylightRenderer() { return SkylightRenderer; }

	DistanceFieldVisualiseSettings_s DistanceFieldVisualise;
	DistanceFieldAOSettings_s DistanceFieldAO;
	SkylightSettings_s Skylight;

	static rl::RootSignature_t GetRootSignature();
	static const rl::GraphicsPipelineTargetDesc& GetMaterialPipelineTargetDesc(SpatialShaderPass_e Pass);
	static const rl::RenderFormat GetMaterialPipelineDepthFormat();

protected:

	RenderGraphResourcePool_s RenderGraphResourcePool;

	SurfClock Clock;

	uint64_t FrameIndex = 0;

	matrix PrevViewProjection;
	bool HasPrevView = false;

	ShadowDenoiser_c ShadowDenoiser;
	SkylightRenderer_c SkylightRenderer;

	DistanceFieldScene_c DistanceFieldScene;
	GlobalDistanceField_c GlobalDistanceField;
	std::vector<DistanceFieldInstance_s> DistanceFieldInstances;

	std::shared_ptr<struct Texture_s> BlueNoiseTexture;
};