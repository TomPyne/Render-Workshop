struct MaterialUniforms_s
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
    uint AlbedoTexture;
    uint MaskTexture;

    uint NormalTexture;
    float3 __Pad;
};

#include "../../../Game/Shaders/MeshMaterial.h"

struct Interpolants_s
{
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
    float4 Tangent : TANGENT;
    float2 UV0 : TEXCOORD0;
    float2 UV1 : TEXCOORD1;
    float2 UV2 : TEXCOORD2;
};

#include "SunTempleShared.h"

#ifdef _VS

void main(in uint VertexID : SV_VertexID, out Interpolants_s Output)
{
    VS_PosNormalTangentUV0(VertexID, Output.Position, Output.Normal, Output.Tangent, Output.UV0);
    Output.UV1 = Output.UV0 * c_Material.ScaleDirt;
    Output.UV2 = Panner2(Output.UV0 * c_Material.EmberAnimScale, c_View.Time, float2(0.1f, 0.033f));
}

#endif // #ifdef _VS

#ifdef _PS

#include "../../../Game/Shaders/Samplers.h"

Texture2D<float4> t_tex2d_f4[8192] : register(t0, space0);

void main(in Interpolants_s Input, out PSOutput_s Output)
{
    float3 Mask = t_tex2d_f4[c_Material.MaskTexture].Sample(SharedWrappedSampler, Input.UV0).xyz;

    float3 Normal = t_tex2d_f4[c_Material.NormalTexture].Sample(SharedWrappedSampler, Input.UV0).xyz * 2.0f - 1.0f;
    float3 TangentNormals = SafeNormalize(Normal);

    float AlbedoAlpha = t_tex2d_f4[c_Material.AlbedoTexture].Sample(SharedWrappedSampler, Input.UV1).x;
    AlbedoAlpha = saturate(pow(AlbedoAlpha, c_Material.DirtContrast) * c_Material.DirtBrightness);
    AlbedoAlpha *= Mask.r;

    float3 Albedo = lerp(c_Material.MetalColor, c_Material.CoalColor, AlbedoAlpha) * AlbedoAlpha;

    float Metallic = lerp(1.0f, 0.0f, Mask.g);

    float RoughnessMetal = lerp(c_Material.RoughnessMetalHigh, c_Material.RoughnessMetalLow, AlbedoAlpha);
    float RoughnessCoal = lerp(c_Material.RoughnessCoalHigh, c_Material.RoughnessCoalLow, AlbedoAlpha);
    float Roughness = lerp(RoughnessMetal, RoughnessCoal, Mask.g);

    float EmissiveAlpha = t_tex2d_f4[c_Material.AlbedoTexture].Sample(SharedWrappedSampler, Input.UV2).x;
    EmissiveAlpha = saturate(lerp(2.0f, 0.0f, EmissiveAlpha));

    float3 EmissiveColor = lerp(c_Material.ColorEmber, c_Material.ColorEmber2, Mask.b);
    EmissiveColor *= EmissiveAlpha * Mask.b * c_Material.Glow;

    MaterialOutput_Default(TangentToWorldNormals(TangentNormals, Input.Normal, Input.Tangent), Output);
    MaterialOutput_Albedo(Albedo, Output);  
    MaterialOutput_Metallic(Metallic, Output);
    MaterialOutput_Roughness(Roughness, Output);
    MaterialOutput_Emissive(EmissiveColor, Output);
}

#endif

