struct MaterialUniforms_s
{   
    float3 ColorMarble1;
    uint UseColoredMarble;

    float3 ColorMarble2;
    uint UseMetalTop;

    float3 ColorMarble2Color1;
    float ScaleBricks1;

    float3 ColorMarble2Color2;
    float AOStrength;   
    
    float NormalIntensity;
    float RoughnessColoredMarble1;
    float RoughnessColoredMarble2;
    float RoughnessMarble1;

    float RoughnessMarble2;
    float ScaleMarble2;
    uint AlbedoTexture;
    uint MaskTexture;

    uint NormalTexture;
    uint DetailNormalTexture;
    float2 __Pad;
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

    Output.UV1 = Output.UV0 / c_Material.ScaleBricks1;
    Output.UV2 = Output.UV0 / c_Material.ScaleMarble2;
}

#endif // #ifdef _VS

#ifdef _PS

#include "../../../Game/Shaders/Samplers.h"

Texture2D<float4> t_tex2d_f4[8192] : register(t0, space0);

void main(in Interpolants_s Input, out PSOutput_s Output)
{
    float3 Normal = t_tex2d_f4[c_Material.NormalTexture].Sample(SharedWrappedSampler, Input.UV0).xyz * 2.0f - 1.0f;
    float3 DetailNormal = t_tex2d_f4[c_Material.DetailNormalTexture].Sample(SharedWrappedSampler, Input.UV2).xyz * 2.0f - 1.0f;
    
    float3 TangentNormals = SafeNormalize(Normal + (DetailNormal * float3(c_Material.NormalIntensity.xx, 0.0f)));

    float4 Mask = t_tex2d_f4[c_Material.MaskTexture].Sample(SharedWrappedSampler, Input.UV0);

    float3 AlbedoSample = t_tex2d_f4[c_Material.AlbedoTexture].Sample(SharedWrappedSampler, Input.UV2).rgb;

    float AO = saturate(c_Material.AOStrength + Mask.r);
    float MarbleAlpha = lerp(-1.0f, 2.0f, AlbedoSample.r);    

    float3 ColorMarble2 = c_Material.ColorMarble2;

    [branch]
    if(c_Material.UseColoredMarble)
    {
        ColorMarble2 = lerp(c_Material.ColorMarble2Color1, c_Material.ColorMarble2Color2, MarbleAlpha);
    }

    float3 ColorMarble = lerp(c_Material.ColorMarble1, ColorMarble2, Mask.g);
    float3 Albedo = AlbedoSample * ColorMarble;

    float Metallic = 0.0f;
    float Roughness = lerp(c_Material.RoughnessMarble1, c_Material.RoughnessMarble2, AlbedoSample.r * Mask.r);

    [branch]
    if(c_Material.UseMetalTop)
    {
        Metallic = lerp(1.0f, 0.0f, Mask.g);

        float Roughness2 = lerp(c_Material.RoughnessColoredMarble1, c_Material.RoughnessColoredMarble2, MarbleAlpha);
        Roughness = lerp(Roughness, Roughness2, Mask.g);
    }

    MaterialOutput_Default(TangentToWorldNormals(TangentNormals, Input.Normal, Input.Tangent), Output);
    MaterialOutput_Metallic(Metallic, Output);
    MaterialOutput_Roughness(Roughness + Mask.a, Output);
    MaterialOutput_Albedo(Albedo * AO, Output);   
}

#endif

