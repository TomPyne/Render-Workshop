#pragma once

#include <Render/RenderTypes.h>
#include <RenderUtils/RenderGraph/RenderGraph.h>
#include <SurfMath.h>

class GlobalDistanceField_c;

struct DistanceFieldAOSettings_s
{
	bool Enabled = true;
	// World distance each cone is traced to
	float MaxDistance = 4.0f;
	// Cone origins are pushed this many voxels out along the normal, so the surface doesn't occlude itself
	float NormalBiasVoxels = 1.0f;
	uint32_t StepsPerCone = 10;
	// Exponent applied to the result, above 1 darkens
	float Power = 1.0f;
};

struct DistanceFieldAORenderer_s
{
	void Init();

	// Returns an R8 occlusion texture the size of OutputSize, 1 is unoccluded. With GlobalVolume NONE or AO disabled, every pixel is 1.
	RenderGraphResourceHandle_t AddPass(RenderGraphBuilder_s& RGBuilder, const DistanceFieldAOSettings_s& Settings, const GlobalDistanceField_c& GlobalDistanceField,
		RenderGraphResourceHandle_t GlobalVolume, RenderGraphResourceHandle_t SceneDepth, RenderGraphResourceHandle_t SceneNormalRoughness, const matrix& InvViewProjection, uint2 OutputSize);

private:
	rl::ComputePipelineStatePtr PSO = {};
};
