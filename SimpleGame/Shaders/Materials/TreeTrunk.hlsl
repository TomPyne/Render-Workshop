struct MaterialUniforms_s
{    
    float3 Color;
    float Roughness;

    uint AlbedoTextureIndex;
    uint NormalTextureIndex;
    float2 __Pad;
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

Texture2D<float4> t_tex2d_f4[8192] : register(t0, space0);

void main(in Interpolants_s Input, out PSOutput_s Output)
{
    float3 Albedo = t_tex2d_f4[c_Material.AlbedoTextureIndex].Sample(SharedWrappedSampler, Input.UV0).rgb;
    float3 TangentNormals1 = t_tex2d_f4[c_Material.NormalTextureIndex].Sample(SharedWrappedSampler, Input.UV0).xyz * 2.0f - 1.0f;
    float3 TangentNormals2 = t_tex2d_f4[c_Material.NormalTextureIndex].Sample(SharedWrappedSampler, Input.UV0 * 0.5f).xyz * 2.0f - 1.0f;
    float3 TangentNormals = TangentNormals1 + (TangentNormals2 * float3(2.0f, 2.0f, 0.0f));

    MaterialOutput_Default(TangentToWorldNormals(normalize(TangentNormals), Input.Normal, Input.Tangent), Output);
    MaterialOutput_Albedo(Albedo.rgb * c_Material.Color, Output);
    MaterialOutput_Metallic(0.0f, Output);
    MaterialOutput_Roughness(c_Material.Roughness, Output);
}

#endif

