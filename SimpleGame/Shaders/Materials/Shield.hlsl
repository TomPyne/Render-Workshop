struct MaterialUniforms_s
{   
    float DirtBrightness;
    float DirtContrast;
    float ScaleDirt;
    float AORoughnessIntensity;

    float3 ColorMetal;
    float RoughnessMetalHigh;
    
    float RoughnessMetalLow;
    uint AlbedoTexture;
    uint MaskTexture;
    uint NormalTexture
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
    Output.UV1 = Output.UV0 * c_Material.ScaleDirt;
}

#endif // #ifdef _VS

#ifdef _PS

#include "../../../Game/Shaders/Samplers.h"

Texture2D<float4> t_tex2d_f4[8192] : register(t0, space0);

void main(in Interpolants_s Input, out PSOutput_s Output)
{
    float Mask = t_tex2d_f4[c_Material.MaskTexture].Sample(SharedWrappedSampler, Input.UV0).r;
    float AlbedoAlpha = t_tex2d_f4[c_Material.AlbedoTexture].Sample(SharedWrappedSampler, Input.UV0).r;
    float3 TangentNormals = t_tex2d_f4[c_Material.NormalTexture].Sample(SharedWrappedSampler, Input.UV0).xyz * 2.0f - 1.0f;

    float3 Albedo = c_Material.ColorMetal * Mask;

    float AO = saturate(Mask + c_Material.AORoughnessIntensity);

    float DirtMask = saturate(pow(AlbedoAlpha, c_Material.DirtContrast) * c_Material.DirtBrightness);

    float Roughness = lerp(c_Material.RoughnessMetalHigh, c_Material.RoughnessMetalLow, AO * DirtMask);

    MaterialOutput_Default(TangentToWorldNormals(TangentNormals, Input.Normal, Input.Tangent), Output);
    MaterialOutput_Metallic(1.0f, Output);
    MaterialOutput_Roughness(Roughness, Output);
    MaterialOutput_Albedo(Albedo, Output);   
}

#endif

