struct MaterialUniforms_s
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
    uint GrassAlbedoTexture;

    uint RocksAlbedoTexture;
    uint GrassNormalTexture;
    uint RocksNormalTexture;
    uint NormalTexture;
};

#include "../../../Game/Shaders/MeshMaterial.h"

struct Interpolants_s
{
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
    float4 Tangent : TANGENT;
    float2 UV0 : TEXCOORD0;
    float2 UV1 : TEXCOORD1;
};

#ifdef _VS

void main(in uint VertexID : SV_VertexID, out Interpolants_s Output)
{
    VS_PosNormalTangentUV0(VertexID, Output.Position, Output.Normal, Output.Tangent, Output.UV0);
    Output.UV1 = Output.UV0 * c_Material.ScaleRocks;
    Output.UV0 *= c_Material.ScaleGrass;
}

#endif // #ifdef _VS

#ifdef _PS

#include "../../../Game/Shaders/Samplers.h"
#include "SunTempleShared.h"

Texture2D<float4> t_tex2d_f4[8192] : register(t0, space0);

void main(in Interpolants_s Input, out PSOutput_s Output)
{
    float BlendAlpha = saturate(pow(normalize(Input.Normal).y, c_Material.BlendPower) * c_Material.BlendMult);
    float3 GrassAlbedo = t_tex2d_f4[c_Material.GrassAlbedoTexture].Sample(SharedWrappedSampler, Input.UV0).xyz * c_Material.GrassColor;
    float3 RockAlbedo = t_tex2d_f4[c_Material.RocksAlbedoTexture].Sample(SharedWrappedSampler, Input.UV1).xyz * c_Material.ColorRocks;
    float3 Albedo = lerp(RockAlbedo, GrassAlbedo, BlendAlpha);

    float3 Normal = t_tex2d_f4[c_Material.NormalTexture].Sample(SharedWrappedSampler, Input.UV0).xyz * 2.0f - 1.0f;
    Normal *= float3(c_Material.NormalIntensityRocks.xx, 1.0f);

    float3 RockDetailNormal = t_tex2d_f4[c_Material.RocksNormalTexture].Sample(SharedWrappedSampler, Input.UV1).xyz * 2.0f - 1.0f;
    RockDetailNormal *= float3(c_Material.DetailNormalIntensityRocks.xx, 1.0f);

    float3 GrassNormal = t_tex2d_f4[c_Material.GrassNormalTexture].Sample(SharedWrappedSampler, Input.UV0).xyz * 2.0f - 1.0f;
    GrassNormal *= float3(c_Material.NormalIntensityGrass.xx, 1.0f);

    float3 TangentNormals = BlendDetailNormals(Normal, lerp(RockDetailNormal, GrassNormal, BlendAlpha));

    float Metallic = lerp(c_Material.MetallicRocksHigh, c_Material.MetallicRocksLow, RockAlbedo.r);
    Metallic = lerp(Metallic, 0.0f, BlendAlpha);

    float Roughness  = lerp(c_Material.RoughnessRocksHigh, c_Material.RoughnessRocksLow, RockAlbedo.r);
    Roughness = lerp(Roughness, c_Material.RoughnessGrass, BlendAlpha);

    MaterialOutput_Default(TangentToWorldNormals(TangentNormals, Input.Normal, Input.Tangent), Output);
    MaterialOutput_Metallic(Metallic, Output);
    MaterialOutput_Roughness(Roughness, Output);
    MaterialOutput_Albedo(Albedo, Output);
}

#endif

