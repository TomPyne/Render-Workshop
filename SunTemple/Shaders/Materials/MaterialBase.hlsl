struct MaterialUniforms_s
{    
    // {MaterialUniforms}
};

#include "../../../Game/Shaders/MeshMaterial.h"

struct Interpolants_s
{
    float4 Position : SV_POSITION;
    float4 PrevPosition : PREV_POSITION;
    float3 Normal : NORMAL;
    float4 Tangent : TANGENT;
#if USE_UV0
    float2 UV0 : TEXCOORD0;
#endif
#if USE_UV1
    float2 UV1 : TEXCOORD1;
#endif
#if USE_UV2
    float2 UV2 : TEXCOORD2;
#endif
};

#ifdef _VS

#if USE_UV0
void SetUV0(float2 UV, inout Interpolants_s Output)
{
    Output.UV0 = UV;
}
#endif

#if USE_UV1
void SetUV1(float2 UV, inout Interpolants_s Output)
{
    Output.UV0 = UV;
}
#endif

#if USE_UV2
void SetUV2(float2 UV, inout Interpolants_s Output)
{
    Output.UV0 = UV;
}
#endif


void main(in uint VertexID : SV_VertexID, out Interpolants_s Output)
{
    VS_PosNormalTangent(VertexID, Output.Position, Output.PrevPosition, Output.Normal, Output.Tangent);

    // {VertexShaderCode}
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

float4 SampleNormalWrapped(uint TextureIndex, float2 UV)
{
    return normalize(t_tex2d_f4[TextureIndex].Sample(SharedWrappedSampler, UV).rgb * 2.0f - 1.0f);
}

float4 SampleNormalClamped(uint TextureIndex, float2 UV)
{
    return normalize(t_tex2d_f4[TextureIndex].Sample(SharedClampedSampler, UV).rgb * 2.0f - 1.0f);
}

void main(in Interpolants_s Input, out PSOutput_s Output)
{
#if !WRITES_NORMALS
    MaterialOutput_Default(normalize(Input.Normal), Output);
#endif
    // {PixelShaderCode}
}

#endif

