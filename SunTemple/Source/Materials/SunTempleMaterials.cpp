#include "SunTempleMaterials.h"

#include <Assets/TextureManager.h>
#include <Shared/FileUtils/PathUtils.h>

ArchMaterialShader_c::ArchMaterialShader_c()
{
    ShaderFilePath = Path_s(PathDirectory_e::Shaders, L"Materials/Arch.hlsl");
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
    ASSIGN_SHADER_PARAM(TextureIndex, MaskTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, AlbedoTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, NormalTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, DetailNormalTexture);
}

void ArchMaterialShader_c::Load()
{
    LoadShaderTexture("MaskTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_Arch_M.hp_tex"));
    LoadShaderTexture("AlbedoTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_FloorMarble_D.hp_tex"));
    LoadShaderTexture("NormalTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_Arch_N.hp_tex"));
    LoadShaderTexture("DetailNormalTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_Marble_N.hp_tex"));
}

BackgroundMatteMaterialShader_c::BackgroundMatteMaterialShader_c()
{
    ShaderFilePath = Path_s(PathDirectory_e::Shaders, L"Materials/BackgroundMatte.hlsl");
    ShaderDebugName = L"BackgroundMatteShader";

    ASSIGN_SHADER_PARAM(float3, Color);
    ASSIGN_SHADER_PARAM(float, Contrast);
    ASSIGN_SHADER_PARAM(float, Brightness);
    ASSIGN_SHADER_PARAM(TextureIndex, MatteTexture);
}

void BackgroundMatteMaterialShader_c::Load()
{
    LoadShaderTexture("MatteTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_BackgroundMatte.hp_tex"));
}

BottomTrimMaterialShader_c::BottomTrimMaterialShader_c()
{
    ShaderFilePath = Path_s(PathDirectory_e::Shaders, L"Materials/BottomTrim.hlsl");
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
    ASSIGN_SHADER_PARAM(DynamicBool, UseUV3);
    ASSIGN_SHADER_PARAM(TextureIndex, MaskTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, AlbedoTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, NormalTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, DetailNormalTexture);
}

void BottomTrimMaterialShader_c::Load()
{
    LoadShaderTexture("MaskTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_BottomTrim_M.hp_tex"));
    LoadShaderTexture("AlbedoTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_FloorMarble_D.hp_tex"));
    LoadShaderTexture("NormalTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_BottomTrim_N.hp_tex"));
    LoadShaderTexture("DetailNormalTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_Marble_N.hp_tex"));
}

TreeTrunkMaterialShader_c::TreeTrunkMaterialShader_c()
{
    ShaderFilePath = Path_s(PathDirectory_e::Shaders, L"Materials/TreeTrunk.hlsl");
    ShaderDebugName = L"TreeTrunkShader";

    ASSIGN_SHADER_PARAM(float3, Color);
    ASSIGN_SHADER_PARAM(float, Roughness);
    ASSIGN_SHADER_PARAM(TextureIndex, AlbedoTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, NormalTexture);
}

void TreeTrunkMaterialShader_c::Load()
{
    LoadShaderTexture("AlbedoTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_Bark01_D.hp_tex"));
    LoadShaderTexture("NormalTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_Bark01_N.hp_tex"));
}

TreeBranchesMaterialShader_c::TreeBranchesMaterialShader_c()
{
    ShaderFilePath = Path_s(PathDirectory_e::Shaders, L"Materials/TreeBranches.hlsl");
    ShaderDebugName = L"TreeBranchShader";

    ASSIGN_SHADER_PARAM(float3, DiffuseColor);
    ASSIGN_SHADER_PARAM(float3, EmissiveColor);
    ASSIGN_SHADER_PARAM(TextureIndex, AlbedoTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, NormalTexture);
}

void TreeBranchesMaterialShader_c::Load()
{
    LoadShaderTexture("AlbedoTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_COG_Foliage_Leaves_D.hp_tex"));
    LoadShaderTexture("NormalTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_COG_Foliage_Leaves_N.hp_tex"));
}

TrimMaterialShader_c::TrimMaterialShader_c()
{
    ShaderFilePath = Path_s(PathDirectory_e::Shaders, L"Materials/Trim.hlsl");
    ShaderDebugName = L"TrimShader";

    ASSIGN_SHADER_PARAM(float3, ColorMarble1);
    ASSIGN_SHADER_PARAM(float, AOStrength);
    ASSIGN_SHADER_PARAM(float3, ColorMarble2);
    ASSIGN_SHADER_PARAM(float, NormalIntensity);
    ASSIGN_SHADER_PARAM(float, RoughnessMarble1);
    ASSIGN_SHADER_PARAM(float, RoughnessMarble2);
    ASSIGN_SHADER_PARAM(float, ScaleMarble1);
    ASSIGN_SHADER_PARAM(float, UTile);
    ASSIGN_SHADER_PARAM(DynamicBool, UseUV3);
    ASSIGN_SHADER_PARAM(TextureIndex, MaskTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, AlbedoTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, NormalTexture);
    ASSIGN_SHADER_PARAM(TextureIndex, DetailNormalTexture);
}

void TrimMaterialShader_c::Load()
{
    LoadShaderTexture("MaskTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_Trim_M.hp_tex"));
    LoadShaderTexture("AlbedoTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_FloorMarble_D.hp_tex"));
    LoadShaderTexture("NormalTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_Trim_N.hp_tex"));
    LoadShaderTexture("DetailNormalTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_Marble_N.hp_tex"));
}

StoneBrickWallMaterialShader_c::StoneBrickWallMaterialShader_c()
{
    ShaderFilePath = Path_s(PathDirectory_e::Shaders, L"Materials/StoneBrickWall.hlsl");
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
    LoadShaderTexture("AOTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_StoneBrickWall_AO.hp_tex"));
    LoadShaderTexture("AlbedoTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_Marble_D.hp_tex"));
    LoadShaderTexture("NormalTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_StoneBrickWall_N.hp_tex"));
    LoadShaderTexture("DetailNormalTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_Marble_N.hp_tex"));
}

SoulRocksMaterialShader_c::SoulRocksMaterialShader_c()
{
    ShaderFilePath = Path_s(PathDirectory_e::Shaders, L"Materials/SoulRocks.hlsl");
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
    LoadShaderTexture("GrassAlbedoTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_Grass_D.hp_tex"));
    LoadShaderTexture("RocksAlbedoTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_Rocks_D.hp_tex"));
    LoadShaderTexture("GrassNormalTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_Grass_N.hp_tex"));
    LoadShaderTexture("RocksNormalTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_Rocks_N.hp_tex"));
    LoadShaderTexture("NormalTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_Cave_Rock_Large02_N.hp_tex"));
}

DomeMaterialShader_c::DomeMaterialShader_c()
{
    ShaderFilePath = Path_s(PathDirectory_e::Shaders, L"Materials/Dome.hlsl");
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
    LoadShaderTexture("MaskTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_Trim_M.hp_tex"));
    LoadShaderTexture("AlbedoTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_FloorMarble_D.hp_tex"));
    LoadShaderTexture("NormalTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_Trim_N.hp_tex"));
    LoadShaderTexture("DetailNormalTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_Marble_N.hp_tex"));
}

FirePitMaterialShader_c::FirePitMaterialShader_c()
{
    ShaderFilePath = Path_s(PathDirectory_e::Shaders, L"Materials/Firepit.hlsl");
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
    LoadShaderTexture("AlbedoTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_FloorMarble_D.hp_tex"));
    LoadShaderTexture("MaskTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_FirePit_M.hp_tex"));
    LoadShaderTexture("NormalTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_FirePit_N.hp_tex"));
}

FloorMaterialShader_c::FloorMaterialShader_c()
{
    ShaderFilePath = Path_s(PathDirectory_e::Shaders, L"Materials/FloorTiles.hlsl");
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
    LoadShaderTexture("PatternMaskTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_FloorTileMasks_M.hp_tex"));
    LoadShaderTexture("AlbedoTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_FloorMarble_D.hp_tex"));
    LoadShaderTexture("NormalTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_FloorTiles_N.hp_tex"));
    LoadShaderTexture("TileMaskTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_FloorTiles_M.hp_tex"));
}

WaterMaterialShader_c::WaterMaterialShader_c()
{
    ShaderFilePath = Path_s(PathDirectory_e::Shaders, L"Materials/Water.hlsl");
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
    LoadShaderTexture("NormalTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_Water2_N.hp_tex"));
}

PillarMaterialShader_c::PillarMaterialShader_c()
{
    ShaderFilePath = Path_s(PathDirectory_e::Shaders, L"Materials/Pillar.hlsl");
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
    LoadShaderTexture("AlbedoTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_FloorMarble_D.hp_tex"));
    LoadShaderTexture("MaskTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_Pillar_M.hp_tex"));
    LoadShaderTexture("NormalTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_Pillar_N.hp_tex"));
    LoadShaderTexture("DetailNormalTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_Marble_N.hp_tex"));
}

RailingMaterialShader_c::RailingMaterialShader_c()
{
    ShaderFilePath = Path_s(PathDirectory_e::Shaders, L"Materials/Railing.hlsl");
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
    LoadShaderTexture("AlbedoTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_FloorMarble_D.hp_tex"));
    LoadShaderTexture("MaskTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_Railing_M.hp_tex"));
    LoadShaderTexture("NormalTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_Railing_N.hp_tex"));
}

SkyMaterialShader_c::SkyMaterialShader_c()
{
    ShaderFilePath = Path_s(PathDirectory_e::Shaders, L"Materials/Sky.hlsl");
    ShaderDebugName = L"SkyShader";

    ASSIGN_SHADER_PARAM(TextureIndex, SkyTexture);
}

void SkyMaterialShader_c::Load()
{
    LoadShaderTexture("SkyTexture", Path_s(PathDirectory_e::Assets, L"Textures/Sky.hp_tex"));
}

StatueMaterialShader_c::StatueMaterialShader_c()
{
    ShaderFilePath = Path_s(PathDirectory_e::Shaders, L"Materials/Statue.hlsl");
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
    LoadShaderTexture("AlbedoTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_Crackle.hp_tex"));
    LoadShaderTexture("MaskTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_Statue_M.hp_tex"));
    LoadShaderTexture("NormalTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_Statue_N.hp_tex"));
    LoadShaderTexture("DetailNormalTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_Crackle_N.hp_tex"));
}

StairsMaterialShader_c::StairsMaterialShader_c()
{
    ShaderFilePath = Path_s(PathDirectory_e::Shaders, L"Materials/Stairs.hlsl");
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
    LoadShaderTexture("AlbedoTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_FloorMarble_D.hp_tex"));
    LoadShaderTexture("MaskTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_Stairs_M.hp_tex"));
    LoadShaderTexture("NormalTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_Stairs_N.hp_tex"));
    LoadShaderTexture("DetailNormalTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_Marble_N.hp_tex"));
}

ShieldMaterialShader_c::ShieldMaterialShader_c()
{
    ShaderFilePath = Path_s(PathDirectory_e::Shaders, L"Materials/Shield.hlsl");
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
    LoadShaderTexture("AlbedoTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_FloorMarble_D.hp_tex"));
    LoadShaderTexture("MaskTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_Shield_M.hp_tex"));
    LoadShaderTexture("NormalTexture", Path_s(PathDirectory_e::Assets, L"Textures/T_Shield_N.hp_tex"));
}

WaveFoamMaterialShader_c::WaveFoamMaterialShader_c()
{
    ShaderFilePath = Path_s(PathDirectory_e::Shaders, L"Materials/WaveFoam.hlsl");
    ShaderDebugName = L"WaveFoamShader";
}

void WaveFoamMaterialShader_c::Load()
{}

SoulTreeMaterialShader_c::SoulTreeMaterialShader_c()
{
    ShaderFilePath = Path_s(PathDirectory_e::Shaders, L"Materials/SoulTree.hlsl");
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
    LoadShaderTexture("AlbedoTexture", Path_s(PathDirectory_e::Assets, L"Textures/Soul_Tree01DF.hp_tex"));
    LoadShaderTexture("NormalTexture", Path_s(PathDirectory_e::Assets, L"Textures/Soul_Tree01NRM.hp_tex"));
}
