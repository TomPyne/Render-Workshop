struct MaterialUniforms_s
{   
    float3 ColorMarble1; 
    float ScaleBricks;

    float AOStrength;
    float NormalIntensity;
    float RoughnessMarble1;
    float RoughnessMarble2;

    float ScaleMarble1;
    uint UseUV0;
    float2 __Pad;

    uint MaskTextureIndex;
    uint AlbedoTextureIndex;
    uint NormalTextureIndex;
    uint DetailNormalTextureIndex;
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
    VS_PosNormalTangentUV0(VertexID, Output.Position, Output.Normal, Output.Tangent, Output.UV0);
    Output.UV1 = (c_Material.UseUV0 ? Output.UV0 : LoadUV1(VertexID)) / c_Material.ScaleMarble1;
    Output.UV2 = Output.UV0 / c_Material.ScaleBricks;
}

#endif // #ifdef _VS

#ifdef _PS

#include "../../../Game/Shaders/Samplers.h"
#include "SunTempleShared.h"

Texture2D<float4> t_tex2d_f4[8192] : register(t0, space0);

void main(in Interpolants_s Input, out PSOutput_s Output)
{
    float3 Albedo = t_tex2d_f4[c_Material.AlbedoTextureIndex].Sample(SharedWrappedSampler, Input.UV1).xyz;
    float Mask = t_tex2d_f4[c_Material.AlbedoTextureIndex].Sample(SharedWrappedSampler, Input.UV0).x;

    float3 Normal = t_tex2d_f4[c_Material.NormalTextureIndex].Sample(SharedWrappedSampler, Input.UV0).xyz * 2.0f - 1.0f;
    float3 DetailNormal = t_tex2d_f4[c_Material.NormalTextureIndex].Sample(SharedWrappedSampler, Input.UV1).xyz * 2.0f - 1.0f;
    DetailNormal *= float3(c_Material.NormalIntensity.xx, 1.0f);
    float3 TangentNormals = BlendDetailNormals(Normal, DetailNormal);

    float Roughness = saturate(lerp(c_Material.RoughnessMarble1, c_Material.RoughnessMarble2, Albedo.r));

    float AO = saturate(Mask + c_Material.AOStrength);

    MaterialOutput_Default(TangentToWorldNormals(TangentNormals, Input.Normal, Input.Tangent), Output);
    MaterialOutput_Metallic(0.0f, Output);
    MaterialOutput_Roughness(Roughness, Output);
    MaterialOutput_Albedo(Albedo * c_Material.ColorMarble1 * AO, Output);   
}

#endif

