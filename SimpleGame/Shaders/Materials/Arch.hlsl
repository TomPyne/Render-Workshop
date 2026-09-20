struct MaterialUniforms_s
{    
    float3 ColorMarble1;
    float AOStrength;

    float3 ColorMarble2;
    float AOStrength2;

    float NormalIntensity;
    float RoughnessMarble1;
    float RoughnessMarble2;
    float ScaleMarble1;

    float ScaleMarble2;
    float3 _Pad0;

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
    float2 UV2 : TEXCOORD2;
};

#ifdef _VS

void main(in uint VertexID : SV_VertexID, out Interpolants_s Output)
{
    Output.Position = ModelToClip(LoadPosition(VertexID));
    Output.Normal = NormalModelToWorld(LoadNormal(VertexID));
    Output.Tangent = TangentModelToWorld(LoadTangent(VertexID));

    Output.UV0 = LoadUV0(VertexID);

    float2 SecondUV = USE_UV3 ? LoadUV2(VertexID) : Output.UV0;

    Output.UV1 = SecondUV / c_Material.ScaleMarble1; // OPT : Pass as reciprocal
    Output.UV2 = SecondUV / c_Material.ScaleMarble2;
}

#endif // #ifdef _VS

#ifdef _PS

#include "../../../Game/Shaders/Samplers.h"
#include "SunTempleShared.h"

Texture2D<float4> t_tex2d_f4[8192] : register(t0, space0);

void main(in Interpolants_s Input, out PSOutput_s Output)
{
    float3 Mask = t_tex2d_f4[c_Material.MaskTextureIndex].Sample(SharedWrappedSampler, Input.UV0).rgb;

    float3 Color = lerp(c_Material.ColorMarble1, c_Material.ColorMarble2, Mask.g);
    float2 CustomUV = lerp(Input.UV1, Input.UV2, Mask.g);
    float3 Albedo = t_tex2d_f4[c_Material.AlbedoTextureIndex].Sample(SharedWrappedSampler, CustomUV).rgb;

    float RoughnessAlpha = Albedo.r * Mask.r;
    float Roughness = lerp(c_Material.RoughnessMarble1, c_Material.RoughnessMarble2, RoughnessAlpha);
    Roughness = saturate(Roughness);

    float AO = saturate((1.0f - Mask.g) + c_Material.AOStrength2);
    AO = saturate(AO + c_Material.AOStrength);

    Albedo = Albedo * Color * AO;

    float3 Normal = t_tex2d_f4[c_Material.NormalTextureIndex].Sample(SharedWrappedSampler, Input.UV0).rgb;
    float3 DetailNormal = t_tex2d_f4[c_Material.DetailNormalIndex].Sample(SharedWrappedSampler, CustomUV).rgb;
    DetailNormal = DetailNormal * float3(c_Material.NormalIntensity, c_Material.NormalIntensity, 1.0f);

    Normal = BlendDetailNormals(Normal, DetailNormal);

    MaterialOutput_Default(TangentToWorldNormals(Normal, Input.Normal, Input.Tangent), Output);
    MaterialOutput_Albedo(Albedo, Output);
    MaterialOutput_Roughness(Roughness, Output);
}

#endif

