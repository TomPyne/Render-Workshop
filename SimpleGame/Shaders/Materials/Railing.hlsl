struct MaterialUniforms_s
{   
    float3 ColorMarble1;
    float AOIntensity;

    float3 ColorMarble2;
    float RoughnessMarbleHigh;

    float RoughnessMarbleLow;
    float ScaleMarble;
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
};

#include "SunTempleShared.h"

#ifdef _VS

void main(in uint VertexID : SV_VertexID, out Interpolants_s Output)
{
    VS_PosNormalTangentUV0(VertexID, Output.Position, Output.Normal, Output.Tangent, Output.UV0);
    Output.UV1 = Output.UV0 * c_Material.ScaleMarble;
}

#endif // #ifdef _VS

#ifdef _PS

#include "../../../Game/Shaders/Samplers.h"

Texture2D<float4> t_tex2d_f4[8192] : register(t0, space0);

void main(in Interpolants_s Input, out PSOutput_s Output)
{
    float3 Normal = t_tex2d_f4[c_Material.NormalTexture].Sample(SharedWrappedSampler, Input.UV0).xyz * 2.0f - 1.0f;
    float3 TangentNormals = SafeNormalize(Normal);

    float Mask = t_tex2d_f4[c_Material.MaskTexture].Sample(SharedWrappedSampler, Input.UV0).r;

    float AlbedoAlpha = t_tex2d_f4[c_Material.AlbedoTexture].Sample(SharedWrappedSampler, Input.UV1).r;
    AlbedoAlpha = lerp(-1.5f, 2.0f, AlbedoAlpha);

    float Roughness = lerp(c_Material.RoughnessMarbleHigh, c_Material.RoughnessMarbleLow, Mask * AlbedoAlpha);

    float3 Albedo = lerp(c_Material.ColorMarble1, c_Material.ColorMarble2, AlbedoAlpha);
    float AO = saturate(c_Material.AOIntensity * Mask);

    MaterialOutput_Default(TangentToWorldNormals(TangentNormals, Input.Normal, Input.Tangent), Output);
    MaterialOutput_Roughness(Roughness, Output);
    MaterialOutput_Albedo(Albedo * AO, Output);   
}

#endif

