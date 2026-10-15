#include "Rendering/Skylight.h"

#include "Rendering/SkylightInfo.h"
#include "Rendering/SpaceRenderer.h"
#include "Rendering/Texture.h"

#include <Render/Render.h>
#include <Render/TextureInfo.h>
#include <RenderUtils/GPUContext/GPUContext.h>
#include <Shared/FileUtils/PathUtils.h>
#include <Shared/Logging/Logging.h>
#include <SurfMath.h>

namespace
{

constexpr rl::RenderFormat kCubeFormat = rl::RenderFormat::R16G16B16A16_FLOAT;

constexpr uint32_t kSourceSize = 512;
constexpr uint32_t kSourceMipCount = 10; // Down to 1x1
constexpr uint32_t kIrradianceSize = 32;
constexpr uint32_t kIrradianceSourceMip = 4; // 32x32 per face
constexpr uint32_t kPrefilterSize = 256;
constexpr uint32_t kPrefilterMipCount = 6; // Down to 8x8, roughness = mip / (count - 1)

uint64_t GetCubeMemoryBytes(uint32_t Size, uint32_t MipCount)
{
	const uint64_t BytesPerTexel = rl::BitsPerPixel(kCubeFormat) / 8;

	uint64_t Bytes = 0;
	for (uint32_t MipIt = 0; MipIt < MipCount; MipIt++)
	{
		const uint64_t MipSize = Max(Size >> MipIt, 1u);
		Bytes += MipSize * MipSize * 6 * BytesPerTexel;
	}
	return Bytes;
}

}

void SkylightRenderer_c::Init()
{
	static const Path_s EquirectToCubePath = Path_s(PathDirectory_e::Shaders, L"Game", L"Skylight/EquirectToCube.hlsl");
	static const Path_s CubeDownsamplePath = Path_s(PathDirectory_e::Shaders, L"Game", L"Skylight/CubeDownsample.hlsl");
	static const Path_s IrradiancePath = Path_s(PathDirectory_e::Shaders, L"Game", L"Skylight/IrradianceConvolve.hlsl");

	rl::ComputePipelineStateDesc PsoDesc = {};
	PsoDesc.RootSignatureOverride = SpaceRenderer_c::GetRootSignature();

	PsoDesc.Cs = rl::CreateComputeShader(EquirectToCubePath.ToString().c_str());
	PsoDesc.DebugName = L"SkylightEquirectToCube";
	EquirectToCubePSO = rl::CreateComputePipelineState(PsoDesc);
	ASSERTMSG(EquirectToCubePSO.IsValid(), "Failed to create Skylight EquirectToCube PSO");

	PsoDesc.Cs = rl::CreateComputeShader(CubeDownsamplePath.ToString().c_str());
	PsoDesc.DebugName = L"SkylightCubeDownsample";
	CubeDownsamplePSO = rl::CreateComputePipelineState(PsoDesc);
	ASSERTMSG(CubeDownsamplePSO.IsValid(), "Failed to create Skylight CubeDownsample PSO");

	PsoDesc.Cs = rl::CreateComputeShader(IrradiancePath.ToString().c_str());
	PsoDesc.DebugName = L"SkylightIrradiance";
	IrradiancePSO = rl::CreateComputePipelineState(PsoDesc);
	ASSERTMSG(IrradiancePSO.IsValid(), "Failed to create Skylight Irradiance PSO");
}

SkylightRenderer_c::Cube_s SkylightRenderer_c::CreateCube(uint32_t Size, uint32_t MipCount, const wchar_t* Name)
{
	Cube_s Cube;
	Cube.Size = Size;
	Cube.MipCount = MipCount;

	// Built by hand as the render graph pool has no mips or cubes. Injected each frame, the graph only tracks its state
	Cube.Texture = std::make_shared<RenderGraphTexture_s>();
	Cube.Texture->Desc.Width = Size;
	Cube.Texture->Desc.Height = Size;
	Cube.Texture->Desc.Dimension = rl::TextureDimension::CUBEMAP;
	Cube.Texture->Desc.Format = kCubeFormat;
	Cube.Texture->Desc.AccessTypes = RenderGraphResourceAccessType_e::SRV_UAV;
	Cube.Texture->AccessTypes = RenderGraphResourceAccessType_e::SRV_UAV;
	Cube.Texture->DebugName = Name;

	rl::TextureCreateDescEx Desc = {};
	Desc.Width = Size;
	Desc.Height = Size;
	Desc.DepthOrArraySize = 6;
	Desc.MipCount = MipCount;
	Desc.Dimension = rl::TextureDimension::CUBEMAP;
	Desc.Flags = rl::RenderResourceFlags::SRV | rl::RenderResourceFlags::UAV;
	Desc.ResourceFormat = kCubeFormat;
	Desc.DebugName = Name;

	Cube.Texture->Texture = rl::CreateTextureEx(Desc);
	Cube.Texture->SRV = rl::CreateTextureSRV(Cube.Texture->Texture, kCubeFormat, rl::TextureDimension::CUBEMAP, MipCount, 1u);

	// Written as a 6 slice array, one view per mip
	Cube.MipUAVs.reserve(MipCount);
	for (uint32_t MipIt = 0; MipIt < MipCount; MipIt++)
	{
		Cube.MipUAVs.emplace_back(rl::CreateTextureUAV(Cube.Texture->Texture, kCubeFormat, rl::TextureDimension::CUBEMAP, 1u, MipIt));
	}

	return Cube;
}

void SkylightRenderer_c::CreateCubes()
{
	SourceCube = CreateCube(kSourceSize, kSourceMipCount, L"SkylightSource");
	IrradianceCube = CreateCube(kIrradianceSize, 1u, L"SkylightIrradiance");
	PrefilterCube = CreateCube(kPrefilterSize, kPrefilterMipCount, L"SkylightPrefilter");
}

void SkylightRenderer_c::AddSourcePasses(RenderGraphBuilder_s& RGBuilder, RenderGraphResourceHandle_t Source, uint32_t EquirectTextureIndex, float Yaw, bool FurnaceTest)
{
	// The per-mip UAVs aren't known to the graph, it only tracks the cube's state as a whole
	std::vector<uint32_t> MipUAVIndices;
	MipUAVIndices.reserve(SourceCube.MipCount);
	for (const rl::UnorderedAccessViewPtr& UAV : SourceCube.MipUAVs)
	{
		MipUAVIndices.push_back(rl::GetDescriptorIndex(UAV));
	}

	const uint32_t Size = SourceCube.Size;
	const rl::ComputePipelineState_t EquirectPSO = EquirectToCubePSO;
	const rl::ComputePipelineState_t DownsamplePSO = CubeDownsamplePSO;
	const rl::Texture_t SourceTexture = SourceCube.Texture->Texture;

	RGBuilder.AddPass(RenderGraphPassType_e::COMPUTE, L"Skylight Equirect To Cube")
	.AccessResource(Source, RenderGraphResourceAccessType_e::UAV, RenderGraphLoadOp_e::DONT_CARE)
	.SetExecuteCallback([=](RenderGraph_s& RG, GPUContext_s& Ctx)
	{
		struct
		{
			uint32_t EquirectTextureIndex;
			uint32_t OutputTextureIndex;
			uint32_t Size;
			float Yaw;

			uint32_t FurnaceTest;
			float __Pad[3];
		} Uniforms = {};

		Uniforms.EquirectTextureIndex = EquirectTextureIndex;
		Uniforms.OutputTextureIndex = MipUAVIndices[0];
		Uniforms.Size = Size;
		Uniforms.Yaw = Yaw;
		Uniforms.FurnaceTest = FurnaceTest ? 1u : 0u;

		Ctx.SetRootSignature(SpaceRenderer_c::GetRootSignature());
		Ctx.SetComputeRootDescriptorTable(SpaceRendererRootSigSlots::RS_UAV_TABLE);
		Ctx.SetComputeRootDescriptorTable(SpaceRendererRootSigSlots::RS_SRV_TABLE);
		Ctx.SetPipelineState(EquirectPSO);
		Ctx.SetComputeRootCBV(SpaceRendererRootSigSlots::RS_DRAWCONSTANTS, RG.Alloc(Uniforms));
		Ctx.Dispatch(DivideRoundUp(Size, 8u), DivideRoundUp(Size, 8u), 6u);
	});

	RGBuilder.AddPass(RenderGraphPassType_e::COMPUTE, L"Skylight Source Mips")
	.AccessResource(Source, RenderGraphResourceAccessType_e::UAV, RenderGraphLoadOp_e::LOAD)
	.SetExecuteCallback([=](RenderGraph_s& RG, GPUContext_s& Ctx)
	{
		Ctx.SetRootSignature(SpaceRenderer_c::GetRootSignature());
		Ctx.SetComputeRootDescriptorTable(SpaceRendererRootSigSlots::RS_UAV_TABLE);
		Ctx.SetPipelineState(DownsamplePSO);

		for (uint32_t MipIt = 1; MipIt < MipUAVIndices.size(); MipIt++)
		{
			struct
			{
				uint32_t SourceTextureIndex;
				uint32_t OutputTextureIndex;
				uint32_t Size;
				uint32_t __Pad;
			} Uniforms = {};

			Uniforms.SourceTextureIndex = MipUAVIndices[MipIt - 1];
			Uniforms.OutputTextureIndex = MipUAVIndices[MipIt];
			Uniforms.Size = Max(Size >> MipIt, 1u);

			Ctx.SetComputeRootCBV(SpaceRendererRootSigSlots::RS_DRAWCONSTANTS, RG.Alloc(Uniforms));
			Ctx.Dispatch(DivideRoundUp(Uniforms.Size, 8u), DivideRoundUp(Uniforms.Size, 8u), 6u);

			// The next mip reads this one
			Ctx.RWBarrier(SourceTexture);
		}
	});
}

void SkylightRenderer_c::AddIrradiancePass(RenderGraphBuilder_s& RGBuilder, RenderGraphResourceHandle_t Source, RenderGraphResourceHandle_t Irradiance)
{
	const uint32_t OutputUAVIndex = rl::GetDescriptorIndex(IrradianceCube.MipUAVs[0]);
	const uint32_t Size = IrradianceCube.Size;
	const uint32_t SourceSize = Max(SourceCube.Size >> kIrradianceSourceMip, 1u);
	const rl::ComputePipelineState_t PSO = IrradiancePSO;

	RGBuilder.AddPass(RenderGraphPassType_e::COMPUTE, L"Skylight Irradiance")
	.AccessResource(Source, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(Irradiance, RenderGraphResourceAccessType_e::UAV, RenderGraphLoadOp_e::DONT_CARE)
	.SetExecuteCallback([=](RenderGraph_s& RG, GPUContext_s& Ctx)
	{
		struct
		{
			uint32_t SourceTextureIndex;
			uint32_t OutputTextureIndex;
			uint32_t Size;
			uint32_t SourceSize;

			float SourceMip;
			float __Pad[3];
		} Uniforms = {};

		Uniforms.SourceTextureIndex = RG.GetSRVIndex(Source);
		Uniforms.OutputTextureIndex = OutputUAVIndex;
		Uniforms.Size = Size;
		Uniforms.SourceSize = SourceSize;
		Uniforms.SourceMip = static_cast<float>(kIrradianceSourceMip);

		Ctx.SetRootSignature(SpaceRenderer_c::GetRootSignature());
		Ctx.SetComputeRootDescriptorTable(SpaceRendererRootSigSlots::RS_UAV_TABLE);
		Ctx.SetComputeRootDescriptorTable(SpaceRendererRootSigSlots::RS_SRV_TABLE);
		Ctx.SetPipelineState(PSO);
		Ctx.SetComputeRootCBV(SpaceRendererRootSigSlots::RS_DRAWCONSTANTS, RG.Alloc(Uniforms));
		Ctx.Dispatch(DivideRoundUp(Size, 8u), DivideRoundUp(Size, 8u), 6u);
	});
}

SkylightOutputs_s SkylightRenderer_c::AddPasses(RenderGraphBuilder_s& RGBuilder, const SkylightInfo_s* Info, const SkylightSettings_s& Settings)
{
	if (!Settings.Enabled || !EquirectToCubePSO.IsValid() || !CubeDownsamplePSO.IsValid() || !IrradiancePSO.IsValid())
	{
		Status = SkylightStatus_e::Disabled;
		return {};
	}

	if (!Info || !Info->EnvironmentMap)
	{
		Status = SkylightStatus_e::NoSkylight;
		return {};
	}

	if (!Info->EnvironmentMap->IsReady())
	{
		Status = SkylightStatus_e::WaitingForTexture;
		return {};
	}

	// Created on first use so a level without a skylight pays nothing
	if (!SourceCube.Texture)
	{
		CreateCubes();
	}

	const float Yaw = Info->Yaw + ConvertToRadians(Settings.YawOffsetDegrees);

	const bool Dirty = !Baked
		|| RebakeRequested
		|| Info->Generation != BakedGeneration
		|| Yaw != BakedYaw
		|| Settings.FurnaceTest != BakedFurnaceTest;

	SkylightOutputs_s Outputs;
	Outputs.Source = RGBuilder.InjectTexture(SourceCube.Texture, L"SkylightSource");
	Outputs.Irradiance = RGBuilder.InjectTexture(IrradianceCube.Texture, L"SkylightIrradiance");
	Outputs.Prefilter = RGBuilder.InjectTexture(PrefilterCube.Texture, L"SkylightPrefilter");
	Outputs.Intensity = Info->Intensity * Settings.IntensityScale;
	Outputs.BackgroundMip = Min(Settings.BackgroundMip, static_cast<float>(SourceCube.MipCount - 1));

	if (Dirty)
	{
		LOGINFO("[SkylightRenderer] Baking skylight, generation %u, yaw %.1f degrees%s", Info->Generation, ConvertToDegrees(Yaw), Settings.FurnaceTest ? ", furnace test" : "");

		AddSourcePasses(RGBuilder, Outputs.Source, rl::GetDescriptorIndex(Info->EnvironmentMap->SRV), Yaw, Settings.FurnaceTest);
		AddIrradiancePass(RGBuilder, Outputs.Source, Outputs.Irradiance);

		Baked = true;
		BakedGeneration = Info->Generation;
		BakedYaw = Yaw;
		BakedFurnaceTest = Settings.FurnaceTest;
		RebakeRequested = false;
	}

	Status = SkylightStatus_e::Baked;

	return Outputs;
}

uint64_t SkylightRenderer_c::GetMemoryBytes() const
{
	if (!SourceCube.Texture)
	{
		return 0;
	}

	return GetCubeMemoryBytes(SourceCube.Size, SourceCube.MipCount)
		+ GetCubeMemoryBytes(IrradianceCube.Size, IrradianceCube.MipCount)
		+ GetCubeMemoryBytes(PrefilterCube.Size, PrefilterCube.MipCount);
}
