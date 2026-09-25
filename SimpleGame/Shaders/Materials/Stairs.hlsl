struct MaterialUniforms_s
{    
    float AOStrength;
    float3 ColorMarble1;

    float NormalIntensity;
    float RoughnessMarble1;
    float RoughnessMarble2;
    float ScaleMarble1;

    uint AlbedoTexture;
    uint MaskTexture;
    uint NormalTexture;
    uint DetailNormalTexture;  
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

#ifdef _VS

void main(in uint VertexID : SV_VertexID, out Interpolants_s Output)
{
    VS_PosNormalTangentUV0(VertexID, Output.Position, Output.Normal, Output.Tangent, Output.UV0);
    Output.UV1 = Output.UV0 / c_Material.ScaleMarble1;
}

#endif // #ifdef _VS

#ifdef _PS

#include "../../../Game/Shaders/Samplers.h"
#include "SunTempleShared.h"

Texture2D<float4> t_tex2d_f4[8192] : register(t0, space0);

void main(in Interpolants_s Input, out PSOutput_s Output)
{
    float Mask = t_tex2d_f4[c_Material.MaskTexture].Sample(SharedWrappedSampler, Input.UV0).r;
    float3 AlbedoSample = t_tex2d_f4[c_Material.AlbedoTexture].Sample(SharedWrappedSampler, Input.UV1).rgb;

    // Base Color
    float AO = saturate(Mask + c_Material.AOStrength);
    float3 BaseColor = AlbedoSample.rgb * c_Material.ColorMarble1 * AO;
    
    // Roughness
    float Roughness = saturate(lerp(c_Material.RoughnessMarble1, c_Material.RoughnessMarble2, Mask * AlbedoSample.r));

    // Normal
    float3 Normal = t_tex2d_f4[c_Material.NormalTexture].Sample(SharedWrappedSampler, Input.UV0).xyz * 2.0f - 1.0f;
    float3 DetailNormal = t_tex2d_f4[c_Material.DetailNormalTexture].Sample(SharedWrappedSampler, Input.UV1).xyz * 2.0f - 1.0f;
    float3 TangentNormals = BlendDetailNormals(Normal, DetailNormal * float3(c_Material.NormalIntensity.xx, 1.0f));

    MaterialOutput_Default(TangentToWorldNormals(normalize(TangentNormals), Input.Normal, Input.Tangent), Output);
    MaterialOutput_Albedo(BaseColor, Output);
    MaterialOutput_Metallic(0.0f, Output);
    MaterialOutput_Roughness(Roughness, Output);
}

#endif

