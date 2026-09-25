struct MaterialUniforms_s
{   
    float3 Color;
    float Distance;

    float Scale1;
    float Scale2;
    float Speed;
    float Speed2;

    uint UseDistanceFade;
    float NormalIntensity;
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
    float2 UV1 : TEXCOORD1;
};

#include "SunTempleShared.h"

#ifdef _VS

void main(in uint VertexID : SV_VertexID, out Interpolants_s Output)
{
    VS_PosNormalTangent(VertexID, Output.Position, Output.Normal, Output.Tangent);
    float2 UV0 = LoadUV0(VertexID);

    Output.UV0 = Panner2(UV0 * c_Material.Scale1, c_View.Time * c_Material.Speed, float2(0.3131f, 0.123f));
    Output.UV1 = Panner2(UV0 * c_Material.Scale2, c_View.Time * c_Material.Speed2, float2(0.114f, 0.2342f));
}

#endif // #ifdef _VS

#ifdef _PS

#include "../../../Game/Shaders/Samplers.h"

Texture2D<float4> t_tex2d_f4[8192] : register(t0, space0);

void main(in Interpolants_s Input, out PSOutput_s Output)
{
    float3 Normal1 = t_tex2d_f4[c_Material.NormalTexture].Sample(SharedWrappedSampler, Input.UV0).xyz * 2.0f - 1.0f;
    float3 Normal2 = t_tex2d_f4[c_Material.NormalTexture].Sample(SharedWrappedSampler, Input.UV1).xyz * 2.0f - 1.0f;
    float3 TangentNormals = BlendDetailNormals(Normal1, Normal2);

    TangenNormals *= float3(c_Material.NormalIntensity.xx, 1.0f);

    float3 Albedo = c_Material.Color;

    [branch]
    if(c_Material.UseDistanceFade)
    {
        float Fade = GetPixelDistance(Input.Position) / c_Material.Distance;
        Fade = 1.0f - saturate(Fade);
        Albedo *= Fade;
    }

    MaterialOutput_Default(TangentToWorldNormals(TangentNormals, Input.Normal, Input.Tangent), Output);
    MaterialOutput_Metallic(1.0f, Output);
    MaterialOutput_Roughness(0.01f, Output);
    MaterialOutput_Albedo(Albedo, Output);   
}

#endif

