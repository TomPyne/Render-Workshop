#pragma once

#include <Render/RenderTypes.h>
#include <RenderUtils/RenderGraph/RenderGraph.h>

#include <cstdint>
#include <vector>

struct SkylightInfo_s;

struct SkylightSettings_s
{
	bool Enabled = true;
	// Multiplies the skylight's own intensity
	float IntensityScale = 1.0f;
	// Added to the skylight's own yaw, changing it rebakes
	float YawOffsetDegrees = 0.0f;
	// Bakes from a constant 1.0 environment, a white rough non-metal should then light to ~1.0
	bool FurnaceTest = false;
	// Source cube mip drawn as the sky, to inspect the chain
	float BackgroundMip = 0.0f;
};

struct SkylightOutputs_s
{
	RenderGraphResourceHandle_t Source = RenderGraphResourceHandle_t::NONE;
	RenderGraphResourceHandle_t Irradiance = RenderGraphResourceHandle_t::NONE;
	RenderGraphResourceHandle_t Prefilter = RenderGraphResourceHandle_t::NONE;
	float Intensity = 0.0f;
	float BackgroundMip = 0.0f;

	bool IsValid() const { return Source != RenderGraphResourceHandle_t::NONE; }
};

enum class SkylightStatus_e : uint32_t
{
	Disabled,
	NoSkylight,
	WaitingForTexture,
	Baked,
};

// Bakes a skylight's equirectangular environment into cubes for image based lighting.
// Only rebakes when the source or a bake setting changes
class SkylightRenderer_c
{
public:
	void Init();

	// Returns invalid outputs until a bake has been recorded, so callers never read an unbaked cube
	SkylightOutputs_s AddPasses(RenderGraphBuilder_s& RGBuilder, const SkylightInfo_s* Info, const SkylightSettings_s& Settings);

	// Rebakes on the next AddPasses
	void RequestRebake() { RebakeRequested = true; }

	SkylightStatus_e GetStatus() const { return Status; }

	// All three cubes, 0 before the first skylight is seen
	uint64_t GetMemoryBytes() const;

private:
	struct Cube_s
	{
		RenderGraphTexturePtr_t Texture = nullptr;
		std::vector<rl::UnorderedAccessViewPtr> MipUAVs;
		uint32_t Size = 0;
		uint32_t MipCount = 0;
	};

	static Cube_s CreateCube(uint32_t Size, uint32_t MipCount, const wchar_t* Name);
	void CreateCubes();

	void AddSourcePasses(RenderGraphBuilder_s& RGBuilder, RenderGraphResourceHandle_t Source, uint32_t EquirectTextureIndex, float Yaw, bool FurnaceTest);
	void AddIrradiancePass(RenderGraphBuilder_s& RGBuilder, RenderGraphResourceHandle_t Source, RenderGraphResourceHandle_t Irradiance);

	rl::ComputePipelineStatePtr EquirectToCubePSO = {};
	rl::ComputePipelineStatePtr CubeDownsamplePSO = {};
	rl::ComputePipelineStatePtr IrradiancePSO = {};

	Cube_s SourceCube;
	Cube_s IrradianceCube;
	Cube_s PrefilterCube;

	// What the cubes currently hold
	bool Baked = false;
	uint32_t BakedGeneration = 0;
	float BakedYaw = 0.0f;
	bool BakedFurnaceTest = false;

	bool RebakeRequested = false;
	SkylightStatus_e Status = SkylightStatus_e::NoSkylight;
};
