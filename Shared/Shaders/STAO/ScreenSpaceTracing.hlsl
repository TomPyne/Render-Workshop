#include "../Util/Random.h"
#include "../Util/Transforms.h"

struct ConstantData
{
    float4x4 Projection;
    float4x4 InverseProjection;
    float4x4 View;

    uint DepthTextureIndex;
    uint NormalTextureIndex;
    uint OutputTextureIndex;
    float __pad0;

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

#define SAMPLE_DEPTH_FUNC(HitPixel) t_tex2d_f1[c_G.DepthTextureIndex].Load(int3(HitPixel, 0))
#include "../ScreenTracing/ScreenTracing.h"

[NumThreads(8, 8, 1)]
void main(uint3 DispatchThreadId : SV_DispatchThreadID)
{
    if(any(DispatchThreadId.xy >= c_G.ViewportSize))
        return;

    const float3 RandomHemiTangentSpace = GetRandomHemisphere_Cosine(float2(DispatchThreadId.xy % 8));    

    const float3 Normal = t_tex2d_f4[c_G.NormalTextureIndex].Load(uint3(DispatchThreadId.xy, 0)).xyz;
    const float3x3 TBN = ComputeBasisMatrix(Normal);

    float3 SampleDirWorldSpace = mul(TBN, RandomHemiTangentSpace);

    float3 DirectionViewSpace = -normalize(mul((float3x3)c_G.View, SampleDirWorldSpace).xyz);

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

    u_tex2d_f4[c_G.OutputTextureIndex][DispatchThreadId.xy] = float4(Hit ? 0.0f.rrr : 1.0f.rrr, 1);
}