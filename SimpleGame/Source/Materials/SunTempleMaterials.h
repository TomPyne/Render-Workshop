#pragma once

#include <Rendering/Materials.h>

class BottomTrimMaterialShader_c : public MaterialShader_c
{
    struct Parameters_s
    {
        float3 ColorMarble1;
        float AOStrength;

        float3 ColorMarble2;
        float RoughnessMetalLow;

        float NormalIntensity;
        float RoughnessMarble1;
        float RoughnessMarble2;
        float ScaleMarble1;

        float3 ColorMetal;
        float RoughnessMetalHigh;

        TextureIndex MaskTexture;
        TextureIndex AlbedoTexture;
        TextureIndex NormalTexture;
        TextureIndex DetailNormalTexture;
    };

public:

    BottomTrimMaterialShader_c();
    virtual void Load() override;
    virtual uint32_t GetShaderParamBufferSize() const override { return static_cast<uint32_t>(sizeof(Parameters_s)); }
    virtual void GetDefaultParams(std::vector<uint8_t>& OutData) const override;
};

class TreeTrunkMaterialShader_c : public MaterialShader_c
{
    struct Parameters_s
    {
        float3 Color;
        float Roughness;
        
        TextureIndex AlbedoTextureIndex;
        TextureIndex NormalTextureIndex;
        float2 __Pad;
    };

public:

    TreeTrunkMaterialShader_c();
    virtual void Load() override;
    virtual uint32_t GetShaderParamBufferSize() const override { return static_cast<uint32_t>(sizeof(Parameters_s)); }
    virtual void GetDefaultParams(std::vector<uint8_t>& OutData) const override;
};

class TreeBranchesMaterialShader_c : public MaterialShader_c
{
    struct Parameters_s
    {
        float3 DiffuseColor;
        TextureIndex AlbedoTextureIndex;

        float3 EmissiveColor;
        TextureIndex NormalTextureIndex;
    };

public:

    TreeBranchesMaterialShader_c();
    virtual void Load() override;
    virtual uint32_t GetShaderParamBufferSize() const override { return static_cast<uint32_t>(sizeof(Parameters_s)); }
    virtual void GetDefaultParams(std::vector<uint8_t>& OutData) const override;
};

class TrimMaterialShader_c : public MaterialShader_c
{
    struct Parameters_s
    {
        float3 ColorMarble1;
        float AOStrength;

        float3 ColorMarble2;
        float NormalIntensity;

        float RoughnessMarble1;
        float RoughnessMarble2;
        float ScaleMarble1;
        float UTile;

        TextureIndex MaskTexture;
        TextureIndex AlbedoTexture;
        TextureIndex NormalTexture;
        TextureIndex DetailNormalTexture;
    };

public:

    TrimMaterialShader_c();
    virtual void Load() override;
    virtual uint32_t GetShaderParamBufferSize() const override { return static_cast<uint32_t>(sizeof(Parameters_s)); }
    virtual void GetDefaultParams(std::vector<uint8_t>& OutData) const override;
};

class StoneBrickWallMaterialShader_c : public MaterialShader_c
{
    struct Parameters_s
    {
        float NormalIntensity;
        float RoughnessMarble1;
        float RoughnessMarble2;
        float __Pad;

        TextureIndex AOTexture;
        TextureIndex AlbedoTexture;
        TextureIndex NormalTexture;
        TextureIndex DetailNormalTexture;
    };

public:

    StoneBrickWallMaterialShader_c();
    virtual void Load() override;
    virtual uint32_t GetShaderParamBufferSize() const override { return static_cast<uint32_t>(sizeof(Parameters_s)); }
    virtual void GetDefaultParams(std::vector<uint8_t>& OutData) const override;
};

class SoulRocksMaterialShader_c : public MaterialShader_c
{
    struct Parameters_s
    {
        float3 GrassColor;
        float NormalIntensity;

        float RoughnessGrass;
        float ScaleGrass;
        float BlendMult;
        float BlendPower;

        float3 ColorRocks;
        float DetailNormalIntensityRocks;

        float MetallicRocksLow;
        float MetallicRocksHigh;
        float NormalIntensityRocks;
        float RoughnessRocksHigh;

        float RoughnessRocksLow;
        float ScaleRocks;
        float NormalIntensityGrass;
        TextureIndex GrassAlbedoTexture;

        TextureIndex RocksAlbedoTexture;
        TextureIndex GrassNormalTexture;
        TextureIndex RocksNormalTexture;
        TextureIndex NormalTexture;
    };

public:

    SoulRocksMaterialShader_c();
    virtual void Load() override;
    virtual uint32_t GetShaderParamBufferSize() const override { return static_cast<uint32_t>(sizeof(Parameters_s)); }
    virtual void GetDefaultParams(std::vector<uint8_t>& OutData) const override;
};

class DomeMaterialShader_c : public MaterialShader_c
{
    struct Parameters_s
    {
        float3 ColorMarble1 = float3(1.0f);
        float ScaleBricks = 1.0f;

        float AOStrength = 0.0f;
        float NormalIntensity = 0.5f;
        float RoughnessMarble1 = 0.3f;
        float RoughnessMarble2 = 0.8f;

        float ScaleMarble1 = 1.0f;
        DynamicBool UseUV0 = true;
        float2 __Pad;

        TextureIndex MaskTexture = 0u;
        TextureIndex AlbedoTexture = 0u;
        TextureIndex NormalTexture = 0u;
        TextureIndex DetailNormalTexture = 0u;
    };

public:

    DomeMaterialShader_c();
    virtual void Load() override;
    virtual uint32_t GetShaderParamBufferSize() const override { return static_cast<uint32_t>(sizeof(Parameters_s)); }
    virtual void GetDefaultParams(std::vector<uint8_t>& OutData) const override;
};

class FirePitMaterialShader_c : public MaterialShader_c
{
    struct Parameters_s
    {
        float3 CoalColor;
        float RoughnessCoalHigh;

        float RoughnessCoalLow;
        float DirtBrightness;
        float DirtContrast;
        float ScaleDirt;

        float3 ColorEmber;
        float EmberAnimScale;

        float3 ColorEmber2;
        float RoughnessMetalHigh;

        float3 MetalColor;
        float RoughnessMetalLow;

        float EmberAnimSpeed;
        float Glow;
        TextureIndex AlbedoTexture;
        TextureIndex MaskTexture;

        TextureIndex NormalTexture;
        float3 __Pad;
    };

public:

    FirePitMaterialShader_c();
    virtual void Load() override;
    virtual uint32_t GetShaderParamBufferSize() const override { return static_cast<uint32_t>(sizeof(Parameters_s)); }
    virtual void GetDefaultParams(std::vector<uint8_t>& OutData) const override;
};

class FloorMaterialShader_c : public MaterialShader_c
{
    struct Parameters_s
    {
        float3 ColorMarble1 = float3(1.0f);
        DynamicBool UseUV2 = 0u;

        float3 ColorMarble2 = float3(1.0f);
        float AOStrength = 0.0f;

        float Roughness1 = 0.0f;
        float Roughness2 = 0.3f;
        float ScaleMarble1 = 1.0f;
        DynamicBool AddSecondMask = 0u;

        float2 OffsetMask = float2(0.0f);
        float2 OffsetMask2 = float2(0.0f);

        float MaskScale = 1.0f;
        float MaskScale2 = 1.0f;
        DynamicBool MaskSwitch = 0u;
        float TilesScale = 1.0f;

        float2 Add1 = float2(0.6f, 0.3f);
        float2 Add2 = float2(0.2f, 0.8f);

        float2 OffsetTiles = float2(0.0f);
        DynamicBool IsFloorTiles2 = 0u;
        TextureIndex PatternMaskTexture = 0u;

        TextureIndex AlbedoTexture = 0u;
        TextureIndex NormalTexture = 0u;
        TextureIndex TileMaskTexture = 0u;
        float __Pad;
    };

public:

    FloorMaterialShader_c();
    virtual void Load() override;
    virtual uint32_t GetShaderParamBufferSize() const override { return static_cast<uint32_t>(sizeof(Parameters_s)); }
    virtual void GetDefaultParams(std::vector<uint8_t>& OutData) const override;
};

class WaterMaterialShader_c : public MaterialShader_c
{
    struct Parameters_s
    {
        float3 Color;
        float Distance;

        float Scale1;
        float Scale2;
        float Speed;
        float Speed2;

        DynamicBool UseDistanceFade;
        float NormalIntensity;
        TextureIndex NormalTexture;
        float __Pad;
    };

public:

    WaterMaterialShader_c();
    virtual void Load() override;
    virtual uint32_t GetShaderParamBufferSize() const override { return static_cast<uint32_t>(sizeof(Parameters_s)); }
    virtual void GetDefaultParams(std::vector<uint8_t>& OutData) const override;
};

class PillarMaterialShader_c : public MaterialShader_c
{
    struct Parameters_s
    {
        float3 ColorMarble1 = float3(1.0f);
        DynamicBool UseColoredMarble = 0;

        float3 ColorMarble2 = float3(1.0f);
        DynamicBool UseMetalTop = 0;

        float3 ColorMarble2Color1 = float3(1.0f);
        float ScaleBricks1 = 1.0f;

        float3 ColorMarble2Color2 = float3(1.0f);
        float AOStrength = 0.0f;

        float NormalIntensity = 0.5f;
        float RoughnessColoredMarble1 = 0.3f;
        float RoughnessColoredMarble2 = 0.8f;
        float RoughnessMarble1 = 0.3f;

        float RoughnessMarble2 = 0.8f;
        float ScaleMarble2 = 1.0f;
        TextureIndex AlbedoTexture = 0;
        TextureIndex MaskTexture = 0;

        TextureIndex NormalTexture = 0;
        TextureIndex DetailNormalTexture = 0;
        float2 __Pad;
    };

public:

    PillarMaterialShader_c();
    virtual void Load() override;
    virtual uint32_t GetShaderParamBufferSize() const override { return static_cast<uint32_t>(sizeof(Parameters_s)); }
    virtual void GetDefaultParams(std::vector<uint8_t>& OutData) const override;
};

class RailingMaterialShader_c : public MaterialShader_c
{
    struct Parameters_s
    {
        float3 ColorMarble1 = float3(0.94f);
        float AOIntensity = 0.0f;

        float3 ColorMarble2 = float3(0.94f);
        float RoughnessMarbleHigh = 0.036f;

        float RoughnessMarbleLow = 0.5f;
        float ScaleMarble = 2.0f;
        TextureIndex AlbedoTexture = 0;
        TextureIndex MaskTexture = 0;

        TextureIndex NormalTexture = 0;
        float3 __Pad;
    };

public:

    RailingMaterialShader_c();
    virtual void Load() override;
    virtual uint32_t GetShaderParamBufferSize() const override { return static_cast<uint32_t>(sizeof(Parameters_s)); }
    virtual void GetDefaultParams(std::vector<uint8_t>& OutData) const override;
};

class StairsMaterialShader_c : public MaterialShader_c
{
    struct Parameters_s
    {
        float AOStrength = 0.0f;
        float3 ColorMarble1 = float3(1.0f);

        float NormalIntensity = 0.5f;
        float RoughnessMarble1 = 0.3f;
        float RoughnessMarble2 = 0.8f;
        float ScaleMarble1 = 1.0f;

        TextureIndex AlbedoTexture = 0u;
        TextureIndex MaskTexture = 0u;
        TextureIndex NormalTexture = 0u;
        TextureIndex DetailNormalTexture = 0u;
    };

public:

    StairsMaterialShader_c();
    virtual void Load() override;
    virtual uint32_t GetShaderParamBufferSize() const override { return static_cast<uint32_t>(sizeof(Parameters_s)); }
    virtual void GetDefaultParams(std::vector<uint8_t>& OutData) const override;
};

class StatueMaterialShader_c : public MaterialShader_c
{
    struct Parameters_s
    {
        float3 Color1 = float3(1.0f);;
        float BrightnessCrackle = 2.0f;

        float3 Color2 = float3(1.0f);;
        float ContrastCrackle = 2.0f;

        float3 Color3 = float3(1.0f);
        float CrackleScale = 1.0f;

        float CrackleShadow = 1.0f;
        float NormalIntensityCrackle = 1.0f;
        float AOIntensity = 0.0f;
        float Metallic = 1.0f;

        float Roughness2High = 0.0f;
        float Roughness2Low = 0.0f;
        float Roughness3High = 0.0f;
        float Roughness3Low = 0.0f;

        float RoughnessHigh = 0.0f;
        float RoughnessLow = 0.0f;
        TextureIndex AlbedoTexture = 0;
        TextureIndex MaskTexture = 0;

        TextureIndex NormalTexture = 0;
        TextureIndex DetailNormalTexture = 0;
    };

public:

    StatueMaterialShader_c();
    virtual void Load() override;
    virtual uint32_t GetShaderParamBufferSize() const override { return static_cast<uint32_t>(sizeof(Parameters_s)); }
    virtual void GetDefaultParams(std::vector<uint8_t>& OutData) const override;
};

class ShieldMaterialShader_c : public MaterialShader_c
{
    struct Parameters_s
    {
        float DirtBrightness = 2.0f;
        float DirtContrast = 2.0f;
        float ScaleDirt = 3.0f;
        float AORoughnessIntensity = 0.0f;

        float3 ColorMetal = float3(1.0f);
        float RoughnessMetalHigh = 0.0f;

        float RoughnessMetalLow = 0.0f;
        TextureIndex AlbedoTexture = 0u;
        TextureIndex MaskTexture = 0u;
        TextureIndex NormalTexture = 0u;
    };

public:

    ShieldMaterialShader_c();
    virtual void Load() override;
    virtual uint32_t GetShaderParamBufferSize() const override { return static_cast<uint32_t>(sizeof(Parameters_s)); }
    virtual void GetDefaultParams(std::vector<uint8_t>& OutData) const override;
};

class WaveFoamMaterialShader_c : public MaterialShader_c
{
    struct Parameters_s
    {

    };

public:

    WaveFoamMaterialShader_c();
    virtual void Load() override;
    virtual uint32_t GetShaderParamBufferSize() const override { return static_cast<uint32_t>(sizeof(Parameters_s)); }
    virtual void GetDefaultParams(std::vector<uint8_t>& OutData) const override;
};

class SoulTreeMaterialShader_c : public MaterialShader_c
{
    struct Parameters_s
    {
        float3 Color = float3(0.178f, 0.23f, 0.03f);
        float NormalIntensity = 1.0f;

        float3 Emissive = float3(0.0f);
        float Roughness = 0.25f;

        float Specular = 0.5f;
        TextureIndex AlbedoTexture = 0u;
        TextureIndex NormalTexture = 0u;
        float __Pad;
    };

public:

    SoulTreeMaterialShader_c();
    virtual void Load() override;
    virtual uint32_t GetShaderParamBufferSize() const override { return static_cast<uint32_t>(sizeof(Parameters_s)); }
    virtual void GetDefaultParams(std::vector<uint8_t>& OutData) const override;
};