#pragma once

#include <Render/RenderTypes.h>
#include <Render/View.h>
#include <RenderUtils/RenderGraph/RenderGraph.h>
#include <SurfMath.h>

#include <cstdint>
#include <vector>

class DistanceFieldScene_c;

struct GlobalDistanceFieldSettings_s
{
	uint32_t Resolution = 128;
	// World size of the volume along each axis
	float Extent = 64.0f;
	// Stored distances are clamped to this many voxels either side of the surface
	float BandVoxels = 4.0f;
	// Keeps the current volume and bounds instead of rebuilding them each frame
	bool Freeze = false;
	// Each voxel only evaluates the instances binned into its brick, rather than every instance
	bool BrickCulling = true;
};

// Instances per brick from the last binning, min and average only count non-empty bricks
struct GlobalDistanceFieldBrickStats_s
{
	uint32_t BrickCount = 0;
	uint32_t EmptyBrickCount = 0;
	uint32_t MinInstances = 0;
	uint32_t MaxInstances = 0;
	float AverageInstances = 0.0f;
	// Length of the flat brick instance list
	uint32_t TotalEntries = 0;
};

// A structured buffer rewritten every frame. An update is copied when the frame's command list runs,
// so there is one buffer per frame in flight, as in DistanceFieldScene_c.
class FrameStructuredBuffer_c
{
public:
	explicit FrameStructuredBuffer_c(uint32_t InStride) : Stride(InStride) {}

	void Update(const void* Data, uint32_t Count);

	// Descriptor index of this frame's buffer, only valid after an Update with Count > 0
	uint32_t GetSRVIndex() const;

private:
	static constexpr uint32_t kBufferCount = rl::RenderView::NumBackBuffers;

	rl::StructuredBufferPtr Buffers[kBufferCount] = {};
	rl::ShaderResourceViewPtr SRVs[kBufferCount] = {};

	// Sized to Capacity elements, since an update always copies the whole buffer
	std::vector<uint8_t> Staging;

	uint32_t Stride = 0;
	uint32_t Capacity = 0;
	uint32_t WriteIndex = 0;
};

// Camera centred, world aligned distance field covering the whole scene near the camera
class GlobalDistanceField_c
{
public:
	// Voxels per brick along each axis, must match kBrickSize in GlobalDistanceFieldComposite.hlsl
	static constexpr uint32_t kBrickSize = 8;

	void Init(rl::RootSignature_t InRootSignature, uint32_t InCBVRootSigSlot, uint32_t InUAVTableRootSigSlot, uint32_t InSRVTableRootSigSlot);

	// Snaps the volume around CameraPos and adds the passes that fill it from Scene's instances. Returns the volume texture.
	RenderGraphResourceHandle_t AddPasses(RenderGraphBuilder_s& RGBuilder, const float3& CameraPos, const DistanceFieldScene_c& Scene);

	const AABB& GetVolumeBounds() const { return VolumeBounds; }
	float GetVoxelSize() const { return Settings.Extent / Settings.Resolution; }
	float GetBand() const { return GetVoxelSize() * Settings.BandVoxels; }
	const GlobalDistanceFieldBrickStats_s& GetBrickStats() const { return BrickStats; }

	GlobalDistanceFieldSettings_s Settings;

private:
	rl::RootSignaturePtr RootSignature = {};
	rl::ComputePipelineStatePtr CompositePSO = {};

	uint32_t CBVRootSigSlot = 0;
	uint32_t UAVTableRootSigSlot = 0;
	uint32_t SRVTableRootSigSlot = 0;

	// Bins each instance into the bricks its world bounds, grown by the band, overlap
	void BinInstances(const DistanceFieldScene_c& Scene);

	RenderGraphTexturePtr_t Volume = {};
	AABB VolumeBounds = {};

	// Per brick (offset, count) into BrickInstances
	std::vector<uint2> BrickRanges;
	std::vector<uint32_t> BrickInstances;

	FrameStructuredBuffer_c BrickRangesBuffer = FrameStructuredBuffer_c(sizeof(uint2));
	FrameStructuredBuffer_c BrickInstancesBuffer = FrameStructuredBuffer_c(sizeof(uint32_t));

	GlobalDistanceFieldBrickStats_s BrickStats;
};
