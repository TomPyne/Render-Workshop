struct MaterialUniforms_s
{    
    float NormalIntensity;
    float RoughnessMarble1;
    float RoughnessMarble2;
    float __Pad;

    uint AOTextureIndex;
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
};

#ifdef _VS

void main(in uint VertexID : SV_VertexID, out Interpolants_s Output)
{
    VS_PosNormalTangentUV0(VertexID, Output.Position, Output.Normal, Output.Tangent, Output.UV0);
}

#endif // #ifdef _VS

#ifdef _PS

#include "../../../Game/Shaders/Samplers.h"
#include "SunTempleShared.h"

Texture2D<float4> t_tex2d_f4[8192] : register(t0, space0);

void main(in Interpolants_s Input, out PSOutput_s Output)
{
    float3 Albedo = t_tex2d_f4[c_Material.AlbedoTextureIndex].Sample(SharedWrappedSampler, Input.UV0).xyz;
    float AO = t_tex2d_f4[c_Material.AOTextureIndex].Sample(SharedWrappedSampler, Input.UV0).x;
    float Roughness = lerp(c_Material.RoughnessMarble1, c_Material.RoughnessMarble2, Albedo.r);
    float3 Normal = t_tex2d_f4[c_Material.NormalTextureIndex].Sample(SharedWrappedSampler, Input.UV0).xyz * 2.0f - 1.0f;
    float3 DetailNormal = t_tex2d_f4[c_Material.NormalTextureIndex].Sample(SharedWrappedSampler, Input.UV0).xyz * 2.0f - 1.0f;

    float3 TangentNormals = Normal + DetailNormal * float3(c_Material.NormalIntensity.xx, 0.0f);

    MaterialOutput_Default(TangentToWorldNormals(TangentNormals, Input.Normal, Input.Tangent), Output);
    MaterialOutput_Metallic(0.0f, Output);
    MaterialOutput_Specular(Albedo.r, Output);
    MaterialOutput_Roughness(Roughness, Output);
    MaterialOutput_Albedo(Albedo * AO, Output);   
}

#endif

