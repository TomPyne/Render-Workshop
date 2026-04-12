#include "../Util/Macros.h"

#ifndef CBV_SLOT
#error Please define CBV_SLOT
#endif

struct Uniforms
{
    uint InputTextureIndex;
    uint OutputTextureIndex;
    uint2 Dim;

    float2 DimRcp;
    float2 SourceDimRcp;

    float Threshold;
    float3 __Pad0;
};

ConstantBuffer<Uniforms> c_G : register(MAKE_CBV_SLOT(CBV_SLOT));

Texture2D<float4> t_tex2d_f4[8192] : register(t0, space0);
RWTexture2D<float4> u_tex2d_f4[8192] : register(u0, space0);

SamplerState ClampedSampler : register(s1);

float RGBToLuminance(float3 RGB)
{
    return dot(float3(0.2126f, 0.7152f, 0.0722f), RGB);
}

// Can improve this using load ops instead of sample
float3 BlurSample(float Threshold, float2 UV)
{
    float3 Sample = t_tex2d_f4[c_G.InputTextureIndex].SampleLevel(ClampedSampler, UV, 0).rgb;
    return RGBToLuminance(Sample) >= Threshold ? Sample : float3(0, 0, 0);
}

[numthreads(8, 8, 1)]
void main(uint3 DispatchThreadId : SV_DispatchThreadID)
{
    if(any(DispatchThreadId.xy >= c_G.Dim))
        return;

    float2 UV = (float2(DispatchThreadId.xy) + 0.5f) * c_G.DimRcp;

    float SampleThreshold = 1.2f;//max(c_G.Threshold, 0.0f);

    float X = c_G.SourceDimRcp.x;
    float Y = c_G.SourceDimRcp.y;
    float X2 = 2.0f * X;
    float Y2 = 2.0f * Y;

    // This whole thing stinks of poor occupancy.
    // Try running just 4x float3s at a time
    float3 A = BlurSample(SampleThreshold, float2(UV.x - X2, UV.y + Y2));
    float3 B = BlurSample(SampleThreshold, float2(UV.x,      UV.y + Y2));
    float3 C = BlurSample(SampleThreshold, float2(UV.x + X2, UV.y + Y2));
                        
    float3 D = BlurSample(SampleThreshold, float2(UV.x - X2, UV.y));
    float3 E = BlurSample(SampleThreshold, float2(UV.x,      UV.y));
    float3 F = BlurSample(SampleThreshold, float2(UV.x + X2, UV.y));

    float3 G = BlurSample(SampleThreshold, float2(UV.x - X2, UV.y - Y2));
    float3 H = BlurSample(SampleThreshold, float2(UV.x,      UV.y - Y2));
    float3 I = BlurSample(SampleThreshold, float2(UV.x + X2, UV.y - Y2));

    float3 J = BlurSample(SampleThreshold, float2(UV.x - X, UV.y + Y));
    float3 K = BlurSample(SampleThreshold, float2(UV.x + X, UV.y + Y));
    float3 L = BlurSample(SampleThreshold, float2(UV.x - X, UV.y - Y));
    float3 M = BlurSample(SampleThreshold, float2(UV.x + X, UV.y - Y));

    float3 Downsampled = E * 0.125f;
    Downsampled += (A+C+G+I)*0.03125f;
    Downsampled += (B+D+F+H)*0.0625f;
    Downsampled += (J+K+L+M)*0.125f;
    //u_tex2d_f4[c_G.OutputTextureIndex][DispatchThreadId.xy] = float4(UV, 0, 1);
    u_tex2d_f4[c_G.OutputTextureIndex][DispatchThreadId.xy] = float4(Downsampled, 1);
}