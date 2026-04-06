#include "../Util/Transforms.h"

struct ConstantData
{
    float4x4  Projection;
    float4x4 InverseProjection;
    float4x4 View;

    uint DepthTextureIndex;
    uint SceneColorTextureIndex;
    uint NormalTextureIndex;
    uint OutputTextureIndex;

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
Texture2D<float4> t_tex2d_f4[8192] : register(t0, space1); // Normal
RWTexture2D<float4> u_tex2d_f4[8192] : register(u0, space0); // Output

SamplerState s_ClampedSampler : register(s1);

#define SAMPLE_DEPTH_FUNC(HitPixel) t_tex2d_f1[c_G.DepthTextureIndex].Load(int3(HitPixel, 0))
#include "../ScreenTracing/ScreenTracing.h"

[NumThreads(8, 8, 1)]
void main(uint3 DispatchThreadId : SV_DispatchThreadID)
{
    if(any(DispatchThreadId.xy >= c_G.ViewportSize))
        return;

    float3 Normal = t_tex2d_f4[c_G.NormalTextureIndex].Load(uint3(DispatchThreadId.xy, 0)).xyz;

    float3 NormalViewSpace = (mul((float3x3)c_G.View, Normal).xyz);

    float2 StartPixel = float2(DispatchThreadId.xy) + 0.5;
    float Depth = t_tex2d_f1[c_G.DepthTextureIndex].Load(uint3(DispatchThreadId.xy, 0));

    float3 OriginViewSpace = GetViewPosFromScreen(StartPixel, Depth, c_G.InverseProjection, c_G.ViewportSizeRcp);

    float3 DirectionViewSpace = normalize(-OriginViewSpace);
    float3 ReflectViewSpace = reflect(DirectionViewSpace, NormalViewSpace);
    ReflectViewSpace.xyz *= -1;

    OriginViewSpace += ReflectViewSpace * 0.1f;

    float3 HitPoint;
    float2 HitPixel;
    bool Hit = TraceScreen(
        c_G.Projection,
        OriginViewSpace,
        ReflectViewSpace,
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

    float3 SceneColOrig = t_tex2d_f4[c_G.SceneColorTextureIndex].Load(uint3(DispatchThreadId.xy, 0)).xyz;

    float3 SceneCol = t_tex2d_f4[c_G.SceneColorTextureIndex].SampleLevel(s_ClampedSampler, HitPixel.xy * c_G.ViewportSizeRcp, 0).xyz;

    SceneCol *= ReflectViewSpace.z < 0.0f ? 0.0f : 1.0f;
    SceneCol *= Hit ? 1.0f : 0.0f;

    SceneCol += SceneColOrig;

    u_tex2d_f4[c_G.OutputTextureIndex][DispatchThreadId.xy] = float4(SceneCol / 2 ,1);
}