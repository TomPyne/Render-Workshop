#include "../Util/Macros.h"

#ifndef CBV_SLOT
#error Please define CBV_SLOT
#endif

struct Uniforms
{
    uint InputTextureIndex;
    uint OutputTextureIndex;
    uint2 ScreenDim;

    float2 ScreenDimRcp;
    float FilterRadius;
    float __Pad0;
};

ConstantBuffer<Uniforms> c_G : register(MAKE_CBV_SLOT(CBV_SLOT));

Texture2D<float4> t_tex2d_f4[8192] : register(t0, space0);
RWTexture2D<float4> u_tex2d_f4[8192] : register(u0, space0);

SamplerState ClampedSampler : register(s1);

float3 BlurSample(float2 UV)
{
    return t_tex2d_f4[c_G.InputTextureIndex].SampleLevel(ClampedSampler, UV, 0).rgb;
}

[numthreads(8, 8, 1)]
void main(uint3 DispatchThreadId : SV_DispatchThreadID)
{
    if(any(DispatchThreadId.xy >= c_G.ScreenDim))
        return;

    float2 UV = (float2(DispatchThreadId.xy) + 0.5f) * c_G.ScreenDimRcp;

    float VPY = UV.y + c_G.FilterRadius;
    float VMY = UV.y - c_G.FilterRadius;

    float UPX = UV.x + c_G.FilterRadius;
    float UMX = UV.x - c_G.FilterRadius;

    float3 A = BlurSample(float2(UMX,   VPY));
    float3 B = BlurSample(float2(UV.x,  VPY));
    float3 C = BlurSample(float2(UPX,   VPY));

    float3 D = BlurSample(float2(UMX,   UV.y));
    float3 E = BlurSample(float2(UV.x,  UV.y));
    float3 F = BlurSample(float2(UPX,   UV.y));

    float3 G = BlurSample(float2(UMX,   VMY));
    float3 H = BlurSample(float2(UV.x,  VMY));
    float3 I = BlurSample(float2(UPX,   VMY));

    float3 Upsampled = E * 4.0f;
    Upsampled += (B+D+F+H)*2.0f;
    Upsampled += (A+C+G+I);
    Upsampled *= (1.0f / 16.0f);
    u_tex2d_f4[c_G.OutputTextureIndex][DispatchThreadId.xy] += float4(Upsampled, 0);
}