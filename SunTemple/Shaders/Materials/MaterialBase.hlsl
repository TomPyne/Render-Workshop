// struct MaterialSettings_s
// {
//     bool UseUV0; // USE_UV0
//     bool UseUV1; // USE_UV1
//     bool UseUV2; // USE_UV2
//     bool UseNormal; // USE_NORMAL
//     bool IsRTShader // RT_SHADER
// };

struct MaterialUniforms_s
{    
    %MaterialUniforms%
};

#include "MaterialBaseShared.h"

#ifdef _VS

void VertexShaderMain(in uint VertexID, inout Interpolants_s Output);

%VertexShaderCode%

void main(in uint VertexID : SV_VertexID, out Interpolants_s Output)
{
    const float3 ModelPosition = LoadPosition(VertexID);
    Position = ModelToClip(ModelPosition);
    PrevPosition = ModelToPrevClip(ModelPosition);

#if USE_NORMAL
    Output.Normal = NormalModelToWorld(LoadNormal(VertexID));
    Output.Tangent = TangentModelToWorld(LoadTangent(VertexID));
#endif // #if USE_NORMAL

#if USE_UV0 && !USE_CUSTOM_UV0
    UV0 = LoadUV0(VertexID);
#endif // #if USE_UV0 && !USE_CUSTOM_UV0

#if USE_UV1 && !USE_CUSTOM_UV1
    UV1 = LoadUV1(VertexID);
#endif // #if USE_UV0 && !USE_CUSTOM_UV0

#if USE_UV2 && !USE_CUSTOM_UV2
    UV2 = LoadUV2(VertexID);
#endif // #if USE_UV0 && !USE_CUSTOM_UV0

    VertexShaderMain(VertexID, Output);
}

#endif // #ifdef _VS

#ifdef _PS

#include "../../../Game/Shaders/Samplers.h"
#include "SunTempleShared.h"

// {Includes}

// {TextureBindings}

Texture2D<float4> t_tex2d_f4[8192] : register(t1, space0);

float4 SampleWrapped(uint TextureIndex, float2 UV)
{
    return t_tex2d_f4[TextureIndex].Sample(SharedWrappedSampler, UV);
}

float4 SampleClamped(uint TextureIndex, float2 UV)
{
    return t_tex2d_f4[TextureIndex].Sample(SharedClampedSampler, UV);
}

float3 SampleNormalWrapped(uint TextureIndex, float2 UV)
{
    return normalize(t_tex2d_f4[TextureIndex].Sample(SharedWrappedSampler, UV).rgb * 2.0f - 1.0f);
}

float3 SampleNormalClamped(uint TextureIndex, float2 UV)
{
    return normalize(t_tex2d_f4[TextureIndex].Sample(SharedClampedSampler, UV).rgb * 2.0f - 1.0f);
}

void PixelShaderMain(in Interpolants_s Input, inout PixelShaderMaterialOutputs_s Output);

void main(in Interpolants_s Input, out PSOutput_s Output)
{
    PixelShaderMaterialOutputs_s MaterialOutputs;
    MaterialOutputs.Albedo = float3(0.0f, 0.0f, 0.0f);
    MaterialOutputs.Metallic = 0.0f;
    MaterialOutputs.TangentNormal = float3(0.0f, 0.0f, 1.0f);
    MaterialOutputs.Roughness = 0.5f;
    MaterialOutputs.Emissive = float3(0.0f, 0.0f, 0.0f);
    MaterialOutputs.Specular = 0.5f;
    MaterialOutputs.AmbientOcclusion = 1.0f;
    
    PixelShaderMain(Input, MaterialOutputs);

    const float3 WorldNormal = TangentToWorldNormals(MaterialOutputs.TangentNormals, Input.Normal, Input.Tangent);

    Output.AlbedoMetallic = float4(MaterialOutputs.Albedo, MaterialOutputs.Metallic);
    Output.NormalRoughness = float4(WorldNormal, MaterialOutputs.Roughness);
    Output.EmissiveSpecular = float4(MaterialOutputs.Emissive, MaterialOutputs.Specular);
    Output.AO = AmbientOcclusion;

    const float2 CurrentUV = SvPosition.xy * c_View.InvViewportSize;

    // Behind the camera last frame, push the previous sample off-screen so temporal effects reject it
    float2 PrevUV = float2(-1.0f, -1.0f);
    float PrevDepth = 0.0f;
    if (PrevPosition.w > 0.0f)
    {
        PrevUV = (PrevPosition.xy / PrevPosition.w) * float2(0.5f, -0.5f) + 0.5f;
        PrevDepth = PrevPosition.w;
    }

    Output.Velocity = float4(CurrentUV - PrevUV, PrevDepth, 0.0f);
}

#endif

