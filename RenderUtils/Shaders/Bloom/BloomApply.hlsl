#include "../Util/Macros.h"

#ifndef CBV_SLOT
#error Please define CBV_SLOT
#endif

struct Uniforms
{
    uint InputTextureIndex;
    uint OutputTextureIndex;
    uint2 ScreenDim;

    float2 SrcDimRcp;
    float BloomIntensity;
    float1 __Pad0;
};

ConstantBuffer<Uniforms> c_G : register(MAKE_CBV_SLOT(CBV_SLOT));

Texture2D<float4> t_tex2d_f4[8192] : register(t0, space0);
RWTexture2D<float4> u_tex2d_f4[8192] : register(u0, space0);

SamplerState ClampedSampler : register(s1);

[numthreads(8, 8, 1)]
void main(uint3 DispatchThreadId : SV_DispatchThreadID)
{
    if(any(DispatchThreadId.xy >= c_G.ScreenDim))
        return;

    float2 SrcUV = (float2(DispatchThreadId.xy) + 0.5f) * c_G.SrcDimRcp;

    float3 Bloom = t_tex2d_f4[c_G.InputTextureIndex].SampleLevel(ClampedSampler, SrcUV, 0).rgb;

    float3 Color = u_tex2d_f4[c_G.OutputTextureIndex][DispatchThreadId.xy].rgb;

    u_tex2d_f4[c_G.OutputTextureIndex][DispatchThreadId.xy] = float4(lerp(Color, Bloom, c_G.BloomIntensity), 1);
}