#pragma once

#include <Render/RenderTypes.h>
#include <RenderUtils/RenderGraph/RenderGraph.h>

class GlobalDistanceField_c;

// Must match the VISUALISE_ defines in DistanceFieldVisualise.hlsl
enum class DistanceFieldVisualiseMode_e : uint32_t
{
	GlobalSlice = 0,
};

struct DistanceFieldVisualiseSettings_s
{
	// Y slice of the global volume shown by GlobalSlice
	uint32_t SliceIndex = 64;
};

struct DistanceFieldVisualiseRenderer_s
{
	void Init(rl::RootSignature_t InRootSignature, uint32_t InCBVRootSigSlot, uint32_t InUAVTableRootSigSlot, uint32_t InSRVTableRootSigSlot);

	// Returns a display ready colour texture the size of OutputSize
	RenderGraphResourceHandle_t AddPass(RenderGraphBuilder_s& RGBuilder, DistanceFieldVisualiseMode_e Mode, const DistanceFieldVisualiseSettings_s& Settings,
		const GlobalDistanceField_c& GlobalDistanceField, RenderGraphResourceHandle_t GlobalVolume, uint2 OutputSize);

private:
	rl::RootSignaturePtr RootSignature = {};
	rl::ComputePipelineStatePtr PSO = {};

	uint32_t CBVRootSigSlot = 0;
	uint32_t UAVTableRootSigSlot = 0;
	uint32_t SRVTableRootSigSlot = 0;
};
