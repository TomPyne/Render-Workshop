#include "../Util/Random.h"
#include "../Util/Transforms.h"

#ifndef CBV_SLOT
#error Please define CBV_SLOT
#endif

struct ConstantData
{
    float4x4 Projection;
    float4x4 InverseProjection;
    float4x4 View;

    uint DepthTextureIndex;
    uint NormalTextureIndex;
    uint OutputTextureIndex;
    uint FrameCount;

    uint VelocityTextureIndex;
    uint ConfidenceTextureIndex;
    uint HistoryTextureIndex;
    float __Pad0;

    float2 ViewportSizeRcp;
    uint2 ViewportSize;

    float Thickness;
	float MaxDistance;
	float MaxSteps;
	float Stride;
	
	float2 DepthProjection;
	float Jitter;
	float NearPlaneZ;
};
ConstantBuffer<ConstantData> c_G : register(CBV_SLOT);

Texture2D<float> t_tex2d_f1[8192] : register(t0, space0); // Depth 
Texture2D<float2> t_tex2d_f2[8192] : register(t0, space1); // Velocity 
Texture2D<float4> t_tex2d_f4[8192] : register(t0, space2); // Normal
RWTexture2D<float> u_tex2d_f1[8192] : register(u0, space0); // Output

#define SAMPLE_DEPTH_FUNC(HitPixel) t_tex2d_f1[c_G.DepthTextureIndex].Load(int3(HitPixel, 0))
#include "../ScreenTracing/ScreenTracing.h"

SamplerState ClampedSampler : register(s1);

[NumThreads(8, 8, 1)]
void main(uint3 DispatchThreadId : SV_DispatchThreadID)
{
    if(any(DispatchThreadId.xy >= c_G.ViewportSize))
        return;

    const float3 RandomHemiTangentSpace = GetRandomHemisphere_Cosine(float2((DispatchThreadId.xy % 8) + c_G.FrameCount * 8));    

    const float3 Normal = t_tex2d_f4[c_G.NormalTextureIndex].Load(uint3(DispatchThreadId.xy, 0)).xyz;
    const float3x3 TBN = ComputeBasisMatrix(Normal);

    float3 SampleDirWorldSpace = mul(TBN, RandomHemiTangentSpace);

    float3 DirectionViewSpace = normalize(mul((float3x3)c_G.View, SampleDirWorldSpace).xyz);

    float2 StartPixel = float2(DispatchThreadId.xy) + 0.5;
    float Depth = t_tex2d_f1[c_G.DepthTextureIndex].Load(uint3(DispatchThreadId.xy, 0));
    float3 OriginViewSpace = GetViewPosFromScreen(StartPixel, Depth, c_G.InverseProjection, c_G.ViewportSizeRcp);

    OriginViewSpace += DirectionViewSpace * 0.1f;

    float3 HitPoint;
    float2 HitPixel;
    bool Hit = TraceScreen(
        c_G.Projection,
        OriginViewSpace,
        DirectionViewSpace,
        c_G.MaxDistance,
        c_G.ViewportSize,
        c_G.DepthProjection,
        c_G.Stride,
        c_G.Jitter,
        c_G.Thickness,
        c_G.MaxSteps,
        c_G.NearPlaneZ,
        HitPoint,
        HitPixel
    );

    Hit = Hit && DirectionViewSpace.z < 0.0f;

    float AO = Hit ? 0.0f : 1.0f;

    // Temporal history recombine

    float Confidence = t_tex2d_f1[c_G.ConfidenceTextureIndex][DispatchThreadId.xy].r;
    float2 Velocity = t_tex2d_f2[c_G.VelocityTextureIndex][DispatchThreadId.xy].rg;

    float2 NDC = (StartPixel * c_G.ViewportSizeRcp) * 2.0f - 1.0f;
    Velocity.y *= -1.0f;
    float2 PrevNDC = NDC - Velocity;

    float2 ReconstructedUv = (PrevNDC * 0.5f) + 0.5f;

    float ReconstructedAO = 0.0f;
    if(all(ReconstructedUv >= 0.0f) && all(ReconstructedUv < 1.0f))
    {
        ReconstructedAO = t_tex2d_f1[c_G.HistoryTextureIndex].SampleLevel(ClampedSampler, ReconstructedUv, 0).r;
    }

    u_tex2d_f1[c_G.OutputTextureIndex][DispatchThreadId.xy] = lerp(AO, ReconstructedAO, Confidence);
}