struct MaterialUniforms_s
{    
    float3 DiffuseColor;
    uint AlbedoTextureIndex;

    float3 EmissiveColor;
    uint NormalTextureIndex;
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
    float4 AlbedoAlpha = t_tex2d_f4[c_Material.AlbedoTextureIndex].Sample(SharedWrappedSampler, Input.UV0);
    if(AlbedoAlpha.a < 0.15)
        discard;

    float3 TangentNormals = t_tex2d_f4[c_Material.NormalTextureIndex].Sample(SharedWrappedSampler, Input.UV0).xyz * 2.0f - 1.0f;

    MaterialOutput_Default(TangentToWorldNormals(normalize(TangentNormals), Input.Normal, Input.Tangent), Output);
    MaterialOutput_Albedo(AlbedoAlpha.rgb * c_Material.DiffuseColor, Output);
    MaterialOutput_Emissive(AlbedoAlpha.rgb * c_Material.EmissiveColor, Output);
}

#endif

