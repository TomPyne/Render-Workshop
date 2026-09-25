struct MaterialUniforms_s
{   
    float3 Color1;
    float BrightnessCrackle;

    float3 Color2;
    float ContrastCrackle;

    float3 Color3;
    float CrackleScale;

    float CrackleShadow;
    float NormalIntensityCrackle;
    float AOIntensity;    
    float Metallic;

    float Roughness2High;
    float Roughness2Low;
    float Roughness3High;
    float Roughness3Low;

    float RoughnessHigh;
    float RoughnessLow;
    uint AlbedoTexture;
    uint MaskTexture;

    uint NormalTexture;
    uint DetailNormalTexture;
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

#include "SunTempleShared.h"

#ifdef _VS

void main(in uint VertexID : SV_VertexID, out Interpolants_s Output)
{
    VS_PosNormalTangentUV0(VertexID, Output.Position, Output.Normal, Output.Tangent, Output.UV0);
    Output.UV1 = Output.UV0 * c_Material.CrackleScale;
}

#endif // #ifdef _VS

#ifdef _PS

#include "../../../Game/Shaders/Samplers.h"

Texture2D<float4> t_tex2d_f4[8192] : register(t0, space0);

void main(in Interpolants_s Input, out PSOutput_s Output)
{
    float3 Mask = t_tex2d_f4[c_Material.MaskTexture].Sample(SharedWrappedSampler, Input.UV0).rgb;

    float3 Normal = t_tex2d_f4[c_Material.NormalTexture].Sample(SharedWrappedSampler, Input.UV0).xyz * 2.0f - 1.0f;

    float3 DetailNormal = t_tex2d_f4[c_Material.DetailNormalTexture].Sample(SharedWrappedSampler, Input.UV1).xyz * 2.0f - 1.0f;
    DetailNormal = lerp(DetailNormal, float3(0.0f, 0.0f, 1.0f), Mask.r);
    DetailNormal *= float3(c_Material.NormalIntensityCrackle.xx, 1.0f);

    float3 TangentNormals = BlendDetailNormals(Normal, DetailNormal);    

    float AlbedoAlpha = t_tex2d_f4[c_Material.AlbedoTexture].Sample(SharedWrappedSampler, Input.UV1).r;

    AlbedoAlpha = lerp(AlbedoAlpha, 1.0f, Mask.r);
    AlbedoAlpha = pow(AlbedoAlpha, c_Material.ContrastCrackle) * c_Material.BrightnessCrackle;

    float Shadow = saturate(c_Material.CrackleShadow + AlbedoAlpha);

    AlbedoAlpha = saturate(AlbedoAlpha);
    float3 ColorMask = saturate(Mask * AlbedoAlpha);

    float3 Albedo = lerp(c_Material.Color1, c_Material.Color2, ColorMask.g);
    Albedo = lerp(Albedo, c_Material.Color3, Mask.b);

    float AO = saturate(c_Material.AOIntensity + Mask.r);

    float Roughness1 = lerp(c_Material.RoughnessHigh, c_Material.RoughnessLow, ColorMask.r);
    float Roughness2 = lerp(c_Material.Roughness2High, c_Material.Roughness2Low, ColorMask.r);
    float Roughness3 = lerp(c_Material.Roughness3High, c_Material.Roughness3Low, ColorMask.r);

    float Roughness = lerp(Roughness1, Roughness2, Mask.g);
    Roughness = lerp(Roughness, Roughness3, Mask.b);

    MaterialOutput_Default(TangentToWorldNormals(TangentNormals, Input.Normal, Input.Tangent), Output);
    MaterialOutput_Metallic(c_Material.Metallic, Output);
    MaterialOutput_Roughness(saturate(Roughness), Output);
    MaterialOutput_Albedo(Albedo * AO * Shadow, Output);   
}

#endif

