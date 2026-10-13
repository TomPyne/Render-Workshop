#include "Rendering/Skylight.h"

#include "Rendering/SkylightInfo.h"
#include "Rendering/Texture.h"

#include <Render/Render.h>
#include <Render/TextureInfo.h>
#include <Shared/Logging/Logging.h>
#include <SurfMath.h>

namespace
{

constexpr rl::RenderFormat kCubeFormat = rl::RenderFormat::R16G16B16A16_FLOAT;

constexpr uint32_t kSourceSize = 512;
constexpr uint32_t kSourceMipCount = 10; // Down to 1x1
constexpr uint32_t kIrradianceSize = 32;
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

SkylightOutputs_s SkylightRenderer_c::AddPasses(RenderGraphBuilder_s& RGBuilder, const SkylightInfo_s* Info, const SkylightSettings_s& Settings)
{
	if (!Settings.Enabled)
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

	if (Dirty)
	{
		LOGINFO("[SkylightRenderer] Baking skylight, generation %u, yaw %.1f degrees%s", Info->Generation, ConvertToDegrees(Yaw), Settings.FurnaceTest ? ", furnace test" : "");

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
