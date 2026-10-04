#pragma once

#include <Render/RenderTypes.h>
#include <RenderUtils/RenderGraph/RenderGraph.h>
#include <SurfMath.h>

struct GlobalDistanceFieldSettings_s
{
	uint32_t Resolution = 128;
	// World size of the volume along each axis
	float Extent = 64.0f;
	// Stored distances are clamped to this many voxels either side of the surface
	float BandVoxels = 4.0f;
};

// Camera centred, world aligned distance field covering the whole scene near the camera
class GlobalDistanceField_c
{
public:
	void Init(rl::RootSignature_t InRootSignature, uint32_t InCBVRootSigSlot, uint32_t InUAVTableRootSigSlot);

	// Snaps the volume around CameraPos and adds the passes that fill it. Returns the volume texture.
	RenderGraphResourceHandle_t AddPasses(RenderGraphBuilder_s& RGBuilder, const float3& CameraPos);

	const AABB& GetVolumeBounds() const { return VolumeBounds; }
	float GetVoxelSize() const { return Settings.Extent / Settings.Resolution; }
	float GetBand() const { return GetVoxelSize() * Settings.BandVoxels; }

	GlobalDistanceFieldSettings_s Settings;

private:
	rl::RootSignaturePtr RootSignature = {};
	rl::ComputePipelineStatePtr CompositePSO = {};

	uint32_t CBVRootSigSlot = 0;
	uint32_t UAVTableRootSigSlot = 0;

	RenderGraphTexturePtr_t Volume = {};
	AABB VolumeBounds = {};
};
