struct MaterialUniforms_s
{   
    float3 Color;
    float NormalIntensity;

    float3 Emissive;
    float Roughness;

    float Specular;
    uint AlbedoTexture;
    uint NormalTexture;
    float __Pad;
};

#include "../../../Game/Shaders/MeshMaterial.h"

struct Interpolants_s
{
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
    float4 Tangent : TANGENT;
    float2 UV0 : TEXCOORD0;
};

#include "SunTempleShared.h"

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
    float4 Albedo = t_tex2d_f4[c_Material.AlbedoTexture].Sample(SharedWrappedSampler, Input.UV0);

    if(Albedo.a < 0.333f)
    {
        discard;
    }

    float3 TangentNormals = t_tex2d_f4[c_Material.AlbedoTexture].Sample(SharedWrappedSampler, Input.UV0).xyz * 2.0f - 1.0f;
    TangentNormals *= float3(c_Material.NormalIntensity.xx, 1.0f);

    MaterialOutput_Default(TangentToWorldNormals(TangentNormals, Input.Normal, Input.Tangent), Output);
    MaterialOutput_Roughness(c_Material.Roughness, Output);
    MaterialOutput_Albedo(Albedo.rgb * c_Material.Color, Output);
    MaterialOutput_Emissive(Albedo.rgb * c_Material.Emissive, Output);
    MaterialOutput_Specular(c_Material.Specular);
}

#endif

