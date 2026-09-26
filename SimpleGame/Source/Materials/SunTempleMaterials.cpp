#include "SunTempleMaterials.h"

#include <Assets/TextureManager.h>
#include <Shared/FileUtils/PathUtils.h>

ArchMaterialShader_c::ArchMaterialShader_c()
{
    ShaderFilePath = L"Materials/Arch.hlsl";
    ShaderDebugName = L"ArchShader";

    ASSIGN_SHADER_PARAM(float3, ColorMarble1);
    ASSIGN_SHADER_PARAM(float, AOStrength);
    ASSIGN_SHADER_PARAM(float3, ColorMarble2);
    ASSIGN_SHADER_PARAM(float, AOStrength2);
    ASSIGN_SHADER_PARAM(float, NormalIntensity);
    ASSIGN_SHADER_PARAM(float, RoughnessMarble1);
    ASSIGN_SHADER_PARAM(float, RoughnessMarble2);
    ASSIGN_SHADER_PARAM(float, ScaleMarble1);
    ASSIGN_SHADER_PARAM(float, ScaleMarble2);
    ASSIGN_SHADER_PARAM(DynamicBool, UseUV3);
    ASSIGN_SHADER_PARAM(TextureIndex, MaskTextureIndex);
    ASSIGN_SHADER_PARAM(TextureIndex, AlbedoTextureIndex);
    ASSIGN_SHADER_PARAM(TextureIndex, NormalTextureIndex);
    ASSIGN_SHADER_PARAM(TextureIndex, DetailNormalTextureIndex);
}

void ArchMaterialShader_c::Load()
{
    AddBindTexture(AlbedoTex, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_FloorMarble_D.hp_tex")));
    AddBindTexture(NormalTex, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_Arch_N.hp_tex")));
    AddBindTexture(DetailNormalTex, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_Marble_N.hp_tex")));
    AddBindTexture(MaskTex, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_Arch_M.hp_tex")));
}

void ArchMaterialShader_c::GetDefaultParams(std::vector<uint8_t>& OutData) const
{
    Parameters_s* const Params = InitDefaultParams<Parameters_s>(OutData);
    Params->AlbedoTextureIndex = GetTextureBindIndex(AlbedoTex);
    Params->NormalTextureIndex = GetTextureBindIndex(NormalTex);
    Params->DetailNormalTextureIndex = GetTextureBindIndex(DetailNormalTex);
    Params->MaskTextureIndex = GetTextureBindIndex(MaskTex);
}


BackgroundMatteMaterialShader_c::BackgroundMatteMaterialShader_c()
{
    ShaderFilePath = L"Materials/BackgroundMatte.hlsl";
    ShaderDebugName = L"BackgroundMatteShader";

    ASSIGN_SHADER_PARAM(float3, Color);
    ASSIGN_SHADER_PARAM(float, Contrast);
    ASSIGN_SHADER_PARAM(float, Brightness);
    ASSIGN_SHADER_PARAM(TextureIndex, MatteTexture);
}

void BackgroundMatteMaterialShader_c::Load()
{
    MatteTexture = LoadBindTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_BackgroundMatte.hp_tex"));
}

void BackgroundMatteMaterialShader_c::GetDefaultParams(std::vector<uint8_t>& OutData) const
{
    Parameters_s* const Params = InitDefaultParams<Parameters_s>(OutData);
    Params->MatteTexture = GetTextureBindIndex(MatteTexture);
}

namespace BottomTrimTextures
{
    enum
    {
        Mask,
        Albedo,
        Normal,
        DetailNormal,
        Count
    };
}

BottomTrimMaterialShader_c::BottomTrimMaterialShader_c()
{
	ShaderFilePath = L"Materials/BottomTrim.hlsl";
	ShaderDebugName = L"BottomTrimShader";

    ASSIGN_SHADER_PARAM(float3, ColorMarble1);
    ASSIGN_SHADER_PARAM(float, AOStrength);
    ASSIGN_SHADER_PARAM(float3, ColorMarble2);
    ASSIGN_SHADER_PARAM(float, RoughnessMetalLow);
    ASSIGN_SHADER_PARAM(float, NormalIntensity);
    ASSIGN_SHADER_PARAM(float, RoughnessMarble1);
    ASSIGN_SHADER_PARAM(float, RoughnessMarble2);
    ASSIGN_SHADER_PARAM(float, ScaleMarble1);
    ASSIGN_SHADER_PARAM(float3, ColorMetal);
    ASSIGN_SHADER_PARAM(float, RoughnessMetalHigh);
    ASSIGN_SHADER_PARAM(TextureIndex, MaskTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, AlbedoTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, NormalTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, DetailNormalTexture);
}

void BottomTrimMaterialShader_c::Load()
{
    AddBindTexture(BottomTrimTextures::Mask, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_BottomTrim_M.hp_tex")));
    AddBindTexture(BottomTrimTextures::Albedo, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_FloorMarble_D.hp_tex")));
    AddBindTexture(BottomTrimTextures::Normal, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_BottomTrim_N.hp_tex")));
    AddBindTexture(BottomTrimTextures::DetailNormal, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_Marble_N.hp_tex")));
}

void BottomTrimMaterialShader_c::GetDefaultParams(std::vector<uint8_t>&OutData) const
{
    Parameters_s* const Params = InitDefaultParams<Parameters_s>(OutData);
    Params->MaskTexture = GetTextureBindIndex(BottomTrimTextures::Mask);
    Params->AlbedoTexture = GetTextureBindIndex(BottomTrimTextures::Albedo);
    Params->NormalTexture = GetTextureBindIndex(BottomTrimTextures::Normal);
    Params->DetailNormalTexture = GetTextureBindIndex(BottomTrimTextures::DetailNormal);
}

namespace TreeTrunkTextures
{
    enum
    {
        Albedo,
        Normal,
        Count
    };
}

TreeTrunkMaterialShader_c::TreeTrunkMaterialShader_c()
{
    ShaderFilePath = L"Materials/TreeTrunk.hlsl";
    ShaderDebugName = L"TreeTrunkShader";

    ASSIGN_SHADER_PARAM(float3, Color);
    ASSIGN_SHADER_PARAM(float, Roughness);
    ASSIGN_SHADER_PARAM(TextureIndex, AlbedoTextureIndex);
    ASSIGN_SHADER_PARAM(TextureIndex, NormalTextureIndex);
}

void TreeTrunkMaterialShader_c::Load()
{
    AddBindTexture(TreeTrunkTextures::Albedo, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_Bark01_D.hp_tex")));
    AddBindTexture(TreeTrunkTextures::Normal, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_Bark01_N.hp_tex")));
}

void TreeTrunkMaterialShader_c::GetDefaultParams(std::vector<uint8_t>&OutData) const
{
    Parameters_s* const Params = InitDefaultParams<Parameters_s>(OutData);
    Params->AlbedoTextureIndex = GetTextureBindIndex(TreeTrunkTextures::Albedo);
    Params->NormalTextureIndex = GetTextureBindIndex(TreeTrunkTextures::Normal);
}

namespace TreeBranchTextures
{
    enum
    {
        Albedo,
        Normal,
        Count
    };
}


TreeBranchesMaterialShader_c::TreeBranchesMaterialShader_c()
{
    ShaderFilePath = L"Materials/TreeBranches.hlsl";
    ShaderDebugName = L"TreeBranchShader";

    ASSIGN_SHADER_PARAM(float3, DiffuseColor);
    ASSIGN_SHADER_PARAM(float3, EmissiveColor);
    ASSIGN_SHADER_PARAM(TextureIndex, AlbedoTextureIndex);
    ASSIGN_SHADER_PARAM(TextureIndex, NormalTextureIndex);
}

void TreeBranchesMaterialShader_c::Load()
{
    AddBindTexture(TreeBranchTextures::Albedo, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_COG_Foliage_Leaves_D.hp_tex")));
    AddBindTexture(TreeBranchTextures::Normal, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_COG_Foliage_Leaves_N.hp_tex")));
}

void TreeBranchesMaterialShader_c::GetDefaultParams(std::vector<uint8_t>&OutData) const
{
    Parameters_s* const Params = InitDefaultParams<Parameters_s>(OutData);
    Params->AlbedoTextureIndex = GetTextureBindIndex(TreeBranchTextures::Albedo);
    Params->NormalTextureIndex = GetTextureBindIndex(TreeBranchTextures::Normal);
}

namespace TrimTextures
{
    enum
    {
        Mask,
        Albedo,
        Normal,
        DetailNormal
    };
}

TrimMaterialShader_c::TrimMaterialShader_c()
{
    ShaderFilePath = L"Materials/Trim.hlsl";
    ShaderDebugName = L"TrimShader";

    ASSIGN_SHADER_PARAM(float3, ColorMarble1);
    ASSIGN_SHADER_PARAM(float, AOStrength);
    ASSIGN_SHADER_PARAM(float3, ColorMarble2);
    ASSIGN_SHADER_PARAM(float, NormalIntensity);
    ASSIGN_SHADER_PARAM(float, RoughnessMarble1);
    ASSIGN_SHADER_PARAM(float, RoughnessMarble2);
    ASSIGN_SHADER_PARAM(float, ScaleMarble1);
    ASSIGN_SHADER_PARAM(float, UTile);
    ASSIGN_SHADER_PARAM(TextureIndex, MaskTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, AlbedoTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, NormalTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, DetailNormalTexture);
}

void TrimMaterialShader_c::Load()
{
    AddBindTexture(TrimTextures::Mask, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_Trim_M.hp_tex")));
    AddBindTexture(TrimTextures::Albedo, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_FloorMarble_D.hp_tex")));
    AddBindTexture(TrimTextures::Normal, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_Trim_N.hp_tex")));
    AddBindTexture(TrimTextures::DetailNormal, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_Marble_N.hp_tex")));
}

void TrimMaterialShader_c::GetDefaultParams(std::vector<uint8_t>&OutData) const
{
    Parameters_s* const Params = InitDefaultParams<Parameters_s>(OutData);
    Params->MaskTexture = GetTextureBindIndex(TrimTextures::Mask);
    Params->AlbedoTexture = GetTextureBindIndex(TrimTextures::Albedo);
    Params->NormalTexture = GetTextureBindIndex(TrimTextures::Normal);
    Params->DetailNormalTexture = GetTextureBindIndex(TrimTextures::DetailNormal);
}

namespace StoneBrickWallTextures
{
    enum
    {
        AO,
        Albedo,
        Normal,
        DetailNormal
    };
}

StoneBrickWallMaterialShader_c::StoneBrickWallMaterialShader_c()
{
    ShaderFilePath = L"Materials/StoneBrickWall.hlsl";
    ShaderDebugName = L"StoneBrickWallShader";

    ASSIGN_SHADER_PARAM(float, NormalIntensity);
    ASSIGN_SHADER_PARAM(float, RoughnessMarble1);
    ASSIGN_SHADER_PARAM(float, RoughnessMarble2);
    ASSIGN_SHADER_PARAM(TextureIndex, AOTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, AlbedoTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, NormalTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, DetailNormalTexture);
}

void StoneBrickWallMaterialShader_c::Load()
{
    AddBindTexture(StoneBrickWallTextures::AO, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_StoneBrickWall_AO.hp_tex")));
    AddBindTexture(StoneBrickWallTextures::Albedo, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_Marble_D.hp_tex")));
    AddBindTexture(StoneBrickWallTextures::Normal, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_StoneBrickWall_N.hp_tex")));
    AddBindTexture(StoneBrickWallTextures::DetailNormal, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_Marble_N.hp_tex")));
}

void StoneBrickWallMaterialShader_c::GetDefaultParams(std::vector<uint8_t>&OutData) const
{
    Parameters_s* const Params = InitDefaultParams<Parameters_s>(OutData);
    Params->AOTexture = GetTextureBindIndex(StoneBrickWallTextures::AO);
    Params->AlbedoTexture = GetTextureBindIndex(StoneBrickWallTextures::Albedo);
    Params->NormalTexture = GetTextureBindIndex(StoneBrickWallTextures::Normal);
    Params->DetailNormalTexture = GetTextureBindIndex(StoneBrickWallTextures::DetailNormal);
}

namespace SoulRocksTextures
{
    enum
    {
        GrassAlbedo,
        RocksAlbedo,
        GrassNormal,
        RocksNormal,
        Normal,
    };
}

SoulRocksMaterialShader_c::SoulRocksMaterialShader_c()
{
    ShaderFilePath = L"Materials/SoulRocks.hlsl";
    ShaderDebugName = L"SoulRocksShader";

    ASSIGN_SHADER_PARAM(float3, GrassColor);
    ASSIGN_SHADER_PARAM(float, NormalIntensity);
    ASSIGN_SHADER_PARAM(float, RoughnessGrass);
    ASSIGN_SHADER_PARAM(float, ScaleGrass);
    ASSIGN_SHADER_PARAM(float, BlendMult);
    ASSIGN_SHADER_PARAM(float, BlendPower);
    ASSIGN_SHADER_PARAM(float3, ColorRocks);
    ASSIGN_SHADER_PARAM(float, DetailNormalIntensityRocks);
    ASSIGN_SHADER_PARAM(float, MetallicRocksLow);
    ASSIGN_SHADER_PARAM(float, MetallicRocksHigh);
    ASSIGN_SHADER_PARAM(float, NormalIntensityRocks);
    ASSIGN_SHADER_PARAM(float, RoughnessRocksHigh);
    ASSIGN_SHADER_PARAM(float, RoughnessRocksLow);
    ASSIGN_SHADER_PARAM(float, ScaleRocks);
    ASSIGN_SHADER_PARAM(float, NormalIntensityGrass);
    ASSIGN_SHADER_PARAM(TextureIndex, GrassAlbedoTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, RocksAlbedoTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, GrassNormalTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, RocksNormalTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, NormalTexture);
}

void SoulRocksMaterialShader_c::Load()
{
    AddBindTexture(SoulRocksTextures::GrassAlbedo, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_Grass_D.hp_tex")));
    AddBindTexture(SoulRocksTextures::RocksAlbedo, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_Rocks_D.hp_tex")));
    AddBindTexture(SoulRocksTextures::GrassNormal, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_Grass_N.hp_tex")));
    AddBindTexture(SoulRocksTextures::RocksNormal, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_Rocks_N.hp_tex")));
    AddBindTexture(SoulRocksTextures::Normal, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_Cave_Rock_Large02_N.hp_tex")));
}

void SoulRocksMaterialShader_c::GetDefaultParams(std::vector<uint8_t>&OutData) const
{
    Parameters_s* const Params = InitDefaultParams<Parameters_s>(OutData);
    Params->GrassAlbedoTexture = GetTextureBindIndex(SoulRocksTextures::GrassAlbedo);
    Params->RocksAlbedoTexture = GetTextureBindIndex(SoulRocksTextures::RocksAlbedo);
    Params->GrassNormalTexture = GetTextureBindIndex(SoulRocksTextures::GrassNormal);
    Params->RocksNormalTexture = GetTextureBindIndex(SoulRocksTextures::RocksNormal);
    Params->NormalTexture = GetTextureBindIndex(SoulRocksTextures::Normal);
}

namespace DomeTextures
{
    enum
    {
        Mask,
        Albedo,
        Normal,
        DetailNormal
    };
}

DomeMaterialShader_c::DomeMaterialShader_c()
{
    ShaderFilePath = L"Materials/Dome.hlsl";
    ShaderDebugName = L"DomeShader";

    ASSIGN_SHADER_PARAM(float3, ColorMarble1);
    ASSIGN_SHADER_PARAM(float, ScaleBricks);
    ASSIGN_SHADER_PARAM(float, AOStrength);
    ASSIGN_SHADER_PARAM(float, NormalIntensity);
    ASSIGN_SHADER_PARAM(float, RoughnessMarble1);
    ASSIGN_SHADER_PARAM(float, RoughnessMarble2);
    ASSIGN_SHADER_PARAM(float, ScaleMarble1);
    ASSIGN_SHADER_PARAM(DynamicBool, UseUV0);
    ASSIGN_SHADER_PARAM(TextureIndex, MaskTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, AlbedoTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, NormalTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, DetailNormalTexture);
}

void DomeMaterialShader_c::Load()
{
    AddBindTexture(DomeTextures::Mask, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_Trim_M.hp_tex")));
    AddBindTexture(DomeTextures::Albedo, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_FloorMarble_D.hp_tex")));
    AddBindTexture(DomeTextures::Normal, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_Trim_N.hp_tex")));
    AddBindTexture(DomeTextures::DetailNormal, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_Marble_N.hp_tex")));
}

void DomeMaterialShader_c::GetDefaultParams(std::vector<uint8_t>&OutData) const
{
    Parameters_s* const Params = InitDefaultParams<Parameters_s>(OutData);
    Params->MaskTexture = GetTextureBindIndex(DomeTextures::Mask);
    Params->AlbedoTexture = GetTextureBindIndex(DomeTextures::Albedo);
    Params->NormalTexture = GetTextureBindIndex(DomeTextures::Normal);
    Params->DetailNormalTexture = GetTextureBindIndex(DomeTextures::DetailNormal);
}

namespace FirePitTextures
{
    enum
    {
        Mask,
        Albedo,
        Normal,
    };
}

FirePitMaterialShader_c::FirePitMaterialShader_c()
{
    ShaderFilePath = L"Materials/Firepit.hlsl";
    ShaderDebugName = L"FirePitShader";

    ASSIGN_SHADER_PARAM(float3, CoalColor);
    ASSIGN_SHADER_PARAM(float, RoughnessCoalHigh);
    ASSIGN_SHADER_PARAM(float, RoughnessCoalLow);
    ASSIGN_SHADER_PARAM(float, DirtBrightness);
    ASSIGN_SHADER_PARAM(float, DirtContrast);
    ASSIGN_SHADER_PARAM(float, ScaleDirt);
    ASSIGN_SHADER_PARAM(float3, ColorEmber);
    ASSIGN_SHADER_PARAM(float, EmberAnimScale);
    ASSIGN_SHADER_PARAM(float3, ColorEmber2);
    ASSIGN_SHADER_PARAM(float, RoughnessMetalHigh);
    ASSIGN_SHADER_PARAM(float3, MetalColor);
    ASSIGN_SHADER_PARAM(float, RoughnessMetalLow);
    ASSIGN_SHADER_PARAM(float, EmberAnimSpeed);
    ASSIGN_SHADER_PARAM(float, Glow);
    ASSIGN_SHADER_PARAM(TextureIndex, AlbedoTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, MaskTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, NormalTexture);
}

void FirePitMaterialShader_c::Load()
{
    AddBindTexture(FirePitTextures::Mask, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_FirePit_M.hp_tex")));
    AddBindTexture(FirePitTextures::Albedo, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_FloorMarble_D.hp_tex")));
    AddBindTexture(FirePitTextures::Normal, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_FirePit_N.hp_tex")));
}

void FirePitMaterialShader_c::GetDefaultParams(std::vector<uint8_t>&OutData) const
{
    Parameters_s* const Params = InitDefaultParams<Parameters_s>(OutData);
    Params->MaskTexture = GetTextureBindIndex(FirePitTextures::Mask);
    Params->AlbedoTexture = GetTextureBindIndex(FirePitTextures::Albedo);
    Params->NormalTexture = GetTextureBindIndex(FirePitTextures::Normal);
}

namespace FloorTextures
{
    enum
    {
        PatternMask,
        Albedo,
        Normal,
        TileMask,
    };
}

FloorMaterialShader_c::FloorMaterialShader_c()
{
    ShaderFilePath = L"Materials/FloorTiles.hlsl";
    ShaderDebugName = L"FloorTilesShader";

    ASSIGN_SHADER_PARAM(float3, ColorMarble1);
    ASSIGN_SHADER_PARAM(DynamicBool, UseUV2);
    ASSIGN_SHADER_PARAM(float3, ColorMarble2);
    ASSIGN_SHADER_PARAM(float, AOStrength);
    ASSIGN_SHADER_PARAM(float, Roughness1);
    ASSIGN_SHADER_PARAM(float, Roughness2);
    ASSIGN_SHADER_PARAM(float, ScaleMarble1);
    ASSIGN_SHADER_PARAM(DynamicBool, AddSecondMask);
    ASSIGN_SHADER_PARAM(float2, OffsetMask);
    ASSIGN_SHADER_PARAM(float2, OffsetMask2);
    ASSIGN_SHADER_PARAM(float, MaskScale);
    ASSIGN_SHADER_PARAM(float, MaskScale2);
    ASSIGN_SHADER_PARAM(DynamicBool, MaskSwitch);
    ASSIGN_SHADER_PARAM(float, TilesScale);
    ASSIGN_SHADER_PARAM(float2, Add1);
    ASSIGN_SHADER_PARAM(float2, Add2);
    ASSIGN_SHADER_PARAM(float2, OffsetTiles);
    ASSIGN_SHADER_PARAM(DynamicBool, IsFloorTiles2);
    ASSIGN_SHADER_PARAM(TextureIndex, PatternMaskTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, AlbedoTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, NormalTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, TileMaskTexture);
}

void FloorMaterialShader_c::Load()
{
    AddBindTexture(FloorTextures::PatternMask, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_FloorTileMasks_M.hp_tex")));
    AddBindTexture(FloorTextures::Albedo, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_FloorMarble_D.hp_tex")));
    AddBindTexture(FloorTextures::Normal, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_FloorTiles_N.hp_tex")));
    AddBindTexture(FloorTextures::TileMask, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_FloorTiles_M.hp_tex")));
}

void FloorMaterialShader_c::GetDefaultParams(std::vector<uint8_t>&OutData) const
{
    Parameters_s* const Params = InitDefaultParams<Parameters_s>(OutData);
    Params->PatternMaskTexture = GetTextureBindIndex(FloorTextures::PatternMask);
    Params->AlbedoTexture = GetTextureBindIndex(FloorTextures::Albedo);
    Params->NormalTexture = GetTextureBindIndex(FloorTextures::Normal);
    Params->TileMaskTexture = GetTextureBindIndex(FloorTextures::TileMask);
}

namespace WaterTextures
{
    enum
    {
        Normal,
    };
}

WaterMaterialShader_c::WaterMaterialShader_c()
{
    ShaderFilePath = L"Materials/Ocean.hlsl";
    ShaderDebugName = L"OceanShader";

    ASSIGN_SHADER_PARAM(float3, Color);
    ASSIGN_SHADER_PARAM(float, Distance);
    ASSIGN_SHADER_PARAM(float, Scale1);
    ASSIGN_SHADER_PARAM(float, Scale2);
    ASSIGN_SHADER_PARAM(float, Speed);
    ASSIGN_SHADER_PARAM(float, Speed2);
    ASSIGN_SHADER_PARAM(DynamicBool, UseDistanceFade);
    ASSIGN_SHADER_PARAM(float, NormalIntensity);
    ASSIGN_SHADER_PARAM(TextureIndex, NormalTexture);
}

void WaterMaterialShader_c::Load()
{
    AddBindTexture(WaterTextures::Normal, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_FloorTileMasks_M.hp_tex")));
}

void WaterMaterialShader_c::GetDefaultParams(std::vector<uint8_t>&OutData) const
{
    Parameters_s* const Params = InitDefaultParams<Parameters_s>(OutData);
    Params->NormalTexture = GetTextureBindIndex(WaterTextures::Normal);
}

namespace PillarTextures
{
    enum
    {
        Albedo,
        Mask,
        Normal,
        DetailNormal,
    };
}

PillarMaterialShader_c::PillarMaterialShader_c()
{
    ShaderFilePath = L"Materials/Pillar.hlsl";
    ShaderDebugName = L"PillarShader";

    ASSIGN_SHADER_PARAM(float3, ColorMarble1);
    ASSIGN_SHADER_PARAM(DynamicBool, UseColoredMarble);
    ASSIGN_SHADER_PARAM(float3, ColorMarble2);
    ASSIGN_SHADER_PARAM(DynamicBool, UseMetalTop);
    ASSIGN_SHADER_PARAM(float3, ColorMarble2Color1);
    ASSIGN_SHADER_PARAM(float, ScaleBricks1);
    ASSIGN_SHADER_PARAM(float3, ColorMarble2Color2);
    ASSIGN_SHADER_PARAM(float, AOStrength);
    ASSIGN_SHADER_PARAM(float, NormalIntensity);
    ASSIGN_SHADER_PARAM(float, RoughnessColoredMarble1);
    ASSIGN_SHADER_PARAM(float, RoughnessColoredMarble2);
    ASSIGN_SHADER_PARAM(float, RoughnessMarble1);
    ASSIGN_SHADER_PARAM(float, RoughnessMarble2);
    ASSIGN_SHADER_PARAM(float, ScaleMarble2);
    ASSIGN_SHADER_PARAM(TextureIndex, AlbedoTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, MaskTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, NormalTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, DetailNormalTexture);
}

void PillarMaterialShader_c::Load()
{
    AddBindTexture(PillarTextures::Albedo, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_FloorMarble_D.hp_tex")));
    AddBindTexture(PillarTextures::Mask, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_Pillar_M.hp_tex")));
    AddBindTexture(PillarTextures::Normal, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_Pillar_N.hp_tex")));
    AddBindTexture(PillarTextures::DetailNormal, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_Marble_N.hp_tex")));
}

void PillarMaterialShader_c::GetDefaultParams(std::vector<uint8_t>& OutData) const
{
    Parameters_s* const Params = InitDefaultParams<Parameters_s>(OutData);
    Params->AlbedoTexture = GetTextureBindIndex(PillarTextures::Albedo);
    Params->MaskTexture = GetTextureBindIndex(PillarTextures::Mask);
    Params->NormalTexture = GetTextureBindIndex(PillarTextures::Normal);
    Params->DetailNormalTexture = GetTextureBindIndex(PillarTextures::DetailNormal);
}

namespace RailingTextures
{
    enum
    {
        Albedo,
        Mask,
        Normal,
    };
}

RailingMaterialShader_c::RailingMaterialShader_c()
{
    ShaderFilePath = L"Materials/Railing.hlsl";
    ShaderDebugName = L"RailingShader";

    ASSIGN_SHADER_PARAM(float3, ColorMarble1);
    ASSIGN_SHADER_PARAM(float, AOIntensity);
    ASSIGN_SHADER_PARAM(float3, ColorMarble2);
    ASSIGN_SHADER_PARAM(float, RoughnessMarbleHigh);
    ASSIGN_SHADER_PARAM(float, RoughnessMarbleLow);
    ASSIGN_SHADER_PARAM(float, ScaleMarble);
    ASSIGN_SHADER_PARAM(TextureIndex, AlbedoTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, MaskTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, NormalTexture);
}

void RailingMaterialShader_c::Load()
{
    AddBindTexture(RailingTextures::Albedo, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_FloorMarble_D.hp_tex")));
    AddBindTexture(RailingTextures::Mask, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_Railing_M.hp_tex")));
    AddBindTexture(RailingTextures::Normal, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_Railing_N.hp_tex")));
}

void RailingMaterialShader_c::GetDefaultParams(std::vector<uint8_t>& OutData) const
{
    Parameters_s* const Params = InitDefaultParams<Parameters_s>(OutData);
    Params->AlbedoTexture = GetTextureBindIndex(RailingTextures::Albedo);
    Params->MaskTexture = GetTextureBindIndex(RailingTextures::Mask);
    Params->NormalTexture = GetTextureBindIndex(RailingTextures::Normal);
}

namespace StatueTextures
{
    enum
    {
        Albedo,
        Mask,
        Normal,
        DetailNormal,
    };
}

StatueMaterialShader_c::StatueMaterialShader_c()
{
    ShaderFilePath = L"Materials/Statue.hlsl";
    ShaderDebugName = L"StatueShader";

    ASSIGN_SHADER_PARAM(float3, Color1);
    ASSIGN_SHADER_PARAM(float, BrightnessCrackle);
    ASSIGN_SHADER_PARAM(float3, Color2);
    ASSIGN_SHADER_PARAM(float, ContrastCrackle);
    ASSIGN_SHADER_PARAM(float3, Color3);
    ASSIGN_SHADER_PARAM(float, CrackleScale);
    ASSIGN_SHADER_PARAM(float, CrackleShadow);
    ASSIGN_SHADER_PARAM(float, NormalIntensityCrackle);
    ASSIGN_SHADER_PARAM(float, AOIntensity);
    ASSIGN_SHADER_PARAM(float, Metallic);
    ASSIGN_SHADER_PARAM(float, Roughness2High);
    ASSIGN_SHADER_PARAM(float, Roughness2Low);
    ASSIGN_SHADER_PARAM(float, Roughness3High);
    ASSIGN_SHADER_PARAM(float, Roughness3Low);
    ASSIGN_SHADER_PARAM(float, RoughnessHigh);
    ASSIGN_SHADER_PARAM(float, RoughnessLow);
    ASSIGN_SHADER_PARAM(TextureIndex, AlbedoTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, MaskTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, NormalTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, DetailNormalTexture);
}

void StatueMaterialShader_c::Load()
{
    AddBindTexture(StatueTextures::Albedo, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_Crackle.hp_tex")));
    AddBindTexture(StatueTextures::Mask, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_Statue_M.hp_tex")));
    AddBindTexture(StatueTextures::Normal, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_Statue_N.hp_tex")));
    AddBindTexture(StatueTextures::DetailNormal, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_Crackle_N.hp_tex")));
}

void StatueMaterialShader_c::GetDefaultParams(std::vector<uint8_t>& OutData) const
{
    Parameters_s* const Params = InitDefaultParams<Parameters_s>(OutData);
    Params->AlbedoTexture = GetTextureBindIndex(StatueTextures::Albedo);
    Params->MaskTexture = GetTextureBindIndex(StatueTextures::Mask);
    Params->NormalTexture = GetTextureBindIndex(StatueTextures::Normal);
    Params->DetailNormalTexture = GetTextureBindIndex(StatueTextures::DetailNormal);
}

namespace StairsTextures
{
    enum
    {
        Albedo,
        Mask,
        Normal,
        DetailNormal,
    };
}

StairsMaterialShader_c::StairsMaterialShader_c()
{
    ShaderFilePath = L"Materials/Stairs.hlsl";
    ShaderDebugName = L"StairsShader";

    ASSIGN_SHADER_PARAM(float, AOStrength);
    ASSIGN_SHADER_PARAM(float3, ColorMarble1);
    ASSIGN_SHADER_PARAM(float, NormalIntensity);
    ASSIGN_SHADER_PARAM(float, RoughnessMarble1);
    ASSIGN_SHADER_PARAM(float, RoughnessMarble2);
    ASSIGN_SHADER_PARAM(float, ScaleMarble1);
    ASSIGN_SHADER_PARAM(TextureIndex, AlbedoTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, MaskTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, NormalTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, DetailNormalTexture);
}

void StairsMaterialShader_c::Load()
{
    AddBindTexture(StairsTextures::Albedo, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_FloorMarble_D.hp_tex")));
    AddBindTexture(StairsTextures::Mask, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_Stairs_M.hp_tex")));
    AddBindTexture(StairsTextures::Normal, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_Stairs_M.hp_tex")));
    AddBindTexture(StairsTextures::DetailNormal, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_Marble_N.hp_tex")));
}

void StairsMaterialShader_c::GetDefaultParams(std::vector<uint8_t>& OutData) const
{
    Parameters_s* const Params = InitDefaultParams<Parameters_s>(OutData);
    Params->AlbedoTexture = GetTextureBindIndex(StairsTextures::Albedo);
    Params->MaskTexture = GetTextureBindIndex(StairsTextures::Mask);
    Params->NormalTexture = GetTextureBindIndex(StairsTextures::Normal);
    Params->DetailNormalTexture = GetTextureBindIndex(StairsTextures::DetailNormal);
}

namespace ShieldTextures
{
    enum
    {
        Albedo,
        Mask,
        Normal,
    };
}

ShieldMaterialShader_c::ShieldMaterialShader_c()
{
    ShaderFilePath = L"Materials/Shield.hlsl";
    ShaderDebugName = L"ShieldShader";

    ASSIGN_SHADER_PARAM(float, DirtBrightness);
    ASSIGN_SHADER_PARAM(float, DirtContrast);
    ASSIGN_SHADER_PARAM(float, ScaleDirt);
    ASSIGN_SHADER_PARAM(float, AORoughnessIntensity);
    ASSIGN_SHADER_PARAM(float3, ColorMetal);
    ASSIGN_SHADER_PARAM(float, RoughnessMetalHigh);
    ASSIGN_SHADER_PARAM(float, RoughnessMetalLow);
    ASSIGN_SHADER_PARAM(TextureIndex, AlbedoTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, MaskTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, NormalTexture);
}

void ShieldMaterialShader_c::Load()
{
    AddBindTexture(ShieldTextures::Albedo, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_FloorMarble_D.hp_tex")));
    AddBindTexture(ShieldTextures::Mask, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_Shield_M.hp_tex")));
    AddBindTexture(ShieldTextures::Normal, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/T_Shield_N.hp_tex")));
}

void ShieldMaterialShader_c::GetDefaultParams(std::vector<uint8_t>& OutData) const
{
    Parameters_s* const Params = InitDefaultParams<Parameters_s>(OutData);
    Params->AlbedoTexture = GetTextureBindIndex(ShieldTextures::Albedo);
    Params->MaskTexture = GetTextureBindIndex(ShieldTextures::Mask);
    Params->NormalTexture = GetTextureBindIndex(ShieldTextures::Normal);
}

namespace WaveFoamTextures
{
    enum
    {

    };
}

WaveFoamMaterialShader_c::WaveFoamMaterialShader_c()
{
    ShaderFilePath = L"Materials/WaveFoam.hlsl";
    ShaderDebugName = L"WaveFoamShader";
}

void WaveFoamMaterialShader_c::Load()
{}

void WaveFoamMaterialShader_c::GetDefaultParams(std::vector<uint8_t>& OutData) const
{
    OutData.resize(sizeof(Parameters_s));
    Parameters_s* Params = reinterpret_cast<Parameters_s*>(OutData.data());
}

namespace SoulTreeTextures
{
    enum
    {
        Albedo,
        Normal,
    };
}

SoulTreeMaterialShader_c::SoulTreeMaterialShader_c()
{
    ShaderFilePath = L"Materials/SoulTree.hlsl";
    ShaderDebugName = L"SoulTreeShader";

    ASSIGN_SHADER_PARAM(float3, Color);
    ASSIGN_SHADER_PARAM(float, NormalIntensity);
    ASSIGN_SHADER_PARAM(float3, Emissive);
    ASSIGN_SHADER_PARAM(float, Roughness);
    ASSIGN_SHADER_PARAM(float, Specular);
    ASSIGN_SHADER_PARAM(TextureIndex, AlbedoTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, NormalTexture);
}

void SoulTreeMaterialShader_c::Load()
{
    AddBindTexture(SoulTreeTextures::Albedo, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/Soul_Tree01DF.hp_tex")));
    AddBindTexture(SoulTreeTextures::Normal, TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Textures/SunTemple/Soul_Tree01NRM.hp_tex")));
}

void SoulTreeMaterialShader_c::GetDefaultParams(std::vector<uint8_t>& OutData) const
{
    Parameters_s* const Params = InitDefaultParams<Parameters_s>(OutData);
    Params->AlbedoTexture = GetTextureBindIndex(SoulTreeTextures::Albedo);
    Params->NormalTexture = GetTextureBindIndex(SoulTreeTextures::Normal);
}
