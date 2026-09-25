struct MaterialUniforms_s
{    
    float3 ColorMarble1;
    uint UseUV2;

    float3 ColorMarble2;
    float AOStrength;
    
    float Roughness1;
    float Roughness2;
    float ScaleMarble1;
    uint AddSecondMask;

    float2 OffsetMask;
    float2 OffsetMask2;

    float MaskScale;
    float MaskScale2;
    uint MaskSwitch;
    float TilesScale;    

    float2 Add1;
    float2 Add2;

    float2 OffsetTiles;
    uint IsFloorTiles2;
    uint PatternMaskTexture;

    uint AlbedoTexture;
    uint NormalTexture;
    uint TileMaskTexture;
    float __Pad;

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
    VS_PosNormalTangentUV0UV1(VertexID, Output.Position, Output.Normal, Output.Tangent, Output.UV0, Output.UV1);
    
    [branch]
    if(c_Material.IsFloorTiles2)
    {
        float2 InUV = c_Material.UseUV2 ? Output.UV1 : Output.UV0;
        Output.UV2 = (Output.UV0 + c_Material.OffsetTiles) / c_Material.TilesScale;
    }
    else
    {
        Output.UV2 = (Output.UV0 + c_Material.OffsetTiles) / c_Material.TilesScale;
    }    
}

#endif // #ifdef _VS

#ifdef _PS

#include "../../../Game/Shaders/Samplers.h"

Texture2D<float4> t_tex2d_f4[8192] : register(t0, space0);

float PatternMaskSample(Interpolants_s Input, float2 OffsetMask, float MaskScale)
{
    float2 UV = c_Material.UseUV2 ? Input.UV1 : Input.UV0;
    UV += OffsetMask;
    UV /= MaskScale;
    float2 Sample = t_tex2d_f4[c_Material.PatternMaskTexture].Sample(SharedWrappedSampler, UV).rg;
    return c_Material.MaskSwitch ? Sample.g : Sample.r;
}

void main(in Interpolants_s Input, out PSOutput_s Output)
{
    float3 TangentNormals = t_tex2d_f4[c_Material.NormalTexture].Sample(SharedWrappedSampler, Input.UV2).xyz * 2.0f - 1.0f;
    float3 Mask = t_tex2d_f4[c_Material.TileMaskTexture].Sample(SharedWrappedSampler, Input.UV2).rgb;

    float2 AlbedoUVs = lerp(Input.UV0, Input.UV0 + c_Material.Add1, Mask.g);
    AlbedoUVs = lerp(AlbedoUVs, Input.UV0 + c_Material.Add2, Mask.b);
    AlbedoUVs /= c_Material.ScaleMarble1;

    float3 AlbedoSample = t_tex2d_f4[c_Material.AlbedoTexture].Sample(SharedWrappedSampler, AlbedoUVs).rgb;

    float RoughnessAlpha = Mask.r * AlbedoSample.r;
    float Roughness = lerp(c_Material.Roughness1, c_Material.Roughness2, RoughnessAlpha);

    float PatternMask = PatternMaskSample(Input, c_Material.OffsetMask, c_Material.MaskScale);    

    [branch]
    if(c_Material.AddSecondMask)
    {
        float PatternMask2 = PatternMaskSample(Input, c_Material.OffsetMask2, c_Material.MaskScale2);
        PatternMask = saturate(PatternMask + PatternMask2);
    }

    PatternMask *= Mask.r;
    float3 Albedo = lerp(c_Material.ColorMarble1, c_Material.ColorMarble2, PatternMask);
    Albedo *= AlbedoSample;

    float AO = saturate(Mask.r + c_Material.AOStrength);

    MaterialOutput_Default(TangentToWorldNormals(TangentNormals, Input.Normal, Input.Tangent), Output);
    MaterialOutput_Albedo(Albedo * AO, Output);  
    MaterialOutput_Metallic(0.0f, Output);
    MaterialOutput_Roughness(Roughness, Output);
}

#endif

