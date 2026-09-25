struct MaterialUniforms_s
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

    uint MaskTextureIndex;
    uint AlbedoTextureIndex;
    uint NormalTextureIndex;
    uint DetailNormalIndex;
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
    VS_PosNormalTangentUV0(VertexId, Output.Position, Output.Normal, Output.Tangent, Output.UV0);

    const float2 SecondUV = USE_UV3 ? LoadUV2(VertexID) : Output.UV0;
    Output.UV1 = SecondUV / c_Material.ScaleMarble1;
}

#endif // #ifdef _VS

#ifdef _PS

#include "../../../Game/Shaders/Samplers.h"
#include "SunTempleShared.h"

Texture2D<float4> t_tex2d_f4[8192] : register(t0, space0);

float3 CalcNormals(Interpolants_s Input)
{
    float3 Normal = t_tex2d_f4[c_Material.NormalTextureIndex].Sample(SharedWrappedSampler, Input.UV0).rgb * 2.0f - 1.0f;
    float3 DetailNormal = t_tex2d_f4[c_Material.DetailNormalTextureIndex].Sample(SharedWrappedSampler, Input.UV1).rgb * 2.0f - 1.0f;
    DetailNormal = DetailNormal * float3(c_Material.NormalIntensity.xx, 1.0f);
    float3  BlendDetailNormals(normalize(Normal), DetailNormal);
}

float CalcRoughness(float MaskR, float AlbedoAlpha, float MetallicAlpa)
{
    float RougnessAlpha = (1.0f - MaskR) + AlbedoAlpha;
    float RoughnessMetal = lerp(c_Material.RoughnessMetalHigh, c_Material.RoughnenssMetalLow, RougnessAlpha);
    float RoughnessMarble = lerp(c_Material.RoughnessMarble1, c_Material.RoughnessMarble2, RougnessAlpha);
    return saturate(lerp(RoughnessMarble, RoughnessMetal, MetallicAlpa));
}

float3 CalcAlbedo(Interpolants_s Input, float AlbedoAlpha, float MetallicAlpa)
{    
    AlbedoAlpha = saturate(lerp(-1.0f, 2.0f, AlbedoAlpha));
    float3 Albedo = lerp(c_Material.ColorMarble1, c_Material.ColorMarble2, AlbedoAlpha);
    Albedo = lerp(Albedo, c_Material.ColorMetal, MetallicAlpa);
    return Albedo;
}

float CalcAO(float MaskR)
{
    return saturate(c_Material.AOStrength + MaskR);
}

void main(in Interpolants_s Input, out PSOutput_s Output)
{
    float3 Mask = t_tex2d_f4[c_Material.MaskTextureIndex].Sample(SharedWrappedSampler, Input.UV0).rgb;
    float AO = CalcAO(Mask.r);
    float MetallicAlpa = Mask.g + Mask.b;
    float AlbedoAlpha = t_tex2d_f4[c_Material.AlbedoTextureIndex].Sample(SharedWrappedSampler, Input.UV1).r;

    float3 TangentNormals = CalcNormals(Input);
    MaterialOutput_Default(TangentToWorldNormals(TangentNormals, Input.Normal, Input.Tangent), Output);
    MaterialOutput_Albedo(CalcAlbedo(Input, AlbedoAlpha, MetallicAlpa) * CalcAO(Mask.r), Output);
    MaterialOutput_Metallic(MetallicAlpa, Output);
    MaterialOutput_Roughness(CalcRoughness(Mask.r, AlbedoAlpha, MetallicAlpa), Output);
    // TODO: AO
}

#endif

