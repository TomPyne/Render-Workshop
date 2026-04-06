#pragma once

#ifndef SAMPLE_DEPTH_FUNC
#error Please define a function to sample depth in the including file above this include
#endif

bool DepthIntersection(float Z, float MinZ, float MaxZ, float Thickness)
{
    return (MaxZ >= Z) && (MinZ - Thickness <= Z);
}

float LinearDepth(float Depth, float2 DepthProjection)
{
	return DepthProjection.y / (Depth - DepthProjection.x);
}

float SqrDist(float2 A, float2 B)
{
    const float2 Delta = B - A;
    return dot(Delta, Delta);
}

void Swap(inout float A, inout float B)
{
    const float T = B;
    B = A;
    A = T;
}

bool TraceScreen(float4x4 Projection, float3 OriginViewSpace, float3 DirectionViewSpace, float MaxDistance, uint2 ViewportSize, float2 DepthProjection, float Stride, float Jitter, float Thickness, float MaxSteps, float NearPlaneZ, out float3 HitPoint, out float2 HitPixel)
{
    float RayLength = ((OriginViewSpace.z + DirectionViewSpace.z * MaxDistance) < NearPlaneZ) ? (NearPlaneZ - OriginViewSpace.z) / DirectionViewSpace.z : MaxDistance;

    float3 EndPointViewSpace = OriginViewSpace + DirectionViewSpace * RayLength;

    HitPixel = float2(-1, -1);

    float4 H0 = mul(Projection, float4(OriginViewSpace, 1.0f));
    float4 H1 = mul(Projection, float4(EndPointViewSpace, 1.0f));

    float K0 = 1.0f / H0.w;
    float K1 = 1.0f / H1.w;

    float3 Q0 = OriginViewSpace * K0;
    float3 Q1 = EndPointViewSpace * K1;

    float2 P0 = H0.xy * K0;
    float2 P1 = H1.xy * K1;

    P0 = P0 * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
    P1 = P1 * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);

    P0.xy *= ViewportSize;
    P1.xy *= ViewportSize;

    // L4

    P1 += float(SqrDist(P0, P1) < 0.0001f ? 0.01f : 0.0f).rr;

    float2 Delta = (P1 - P0);

    bool Permute = false;
    if(abs(Delta.x) < abs(Delta.y))
    {
        Permute = true;
        Delta = Delta.yx;
        P0 = P0.yx;
        P1 = P1.yx;
    }

    float StepDir = sign(Delta.x);
    float InvDx = StepDir / Delta.x;

    float3 DerivQ = (Q1 - Q0) * InvDx;
    float DerivK = (K1 - K0) * InvDx;
    float2 DerivP = float2(StepDir, Delta.y * InvDx);

    DerivP *= Stride;
    DerivQ *= Stride;
    DerivK *= Stride;
    P0 += DerivP * Jitter;
    Q0 += DerivQ * Jitter;
    K0 += DerivK * Jitter;

    float PrevZMaxEst = OriginViewSpace.z;

    float2 P = P0;
    float3 Q = Q0;
    float K = K0;
    float StepCount = 0.0f;
    float RayZMax = PrevZMaxEst;
    float RayZMin = PrevZMaxEst;
    float SceneZMax = RayZMax + 1e4;

    float End = P1.x * StepDir;
    
    for(; ((P.x * StepDir) <= End) && (StepCount < MaxSteps) && !DepthIntersection(SceneZMax, RayZMin, RayZMax, Thickness) && (SceneZMax != 0.0f); P += DerivP, Q.z += DerivQ.z, K += DerivK, StepCount += 1.0f)
    {
        HitPixel = Permute ? P.yx : P;

        RayZMin = PrevZMaxEst;
        RayZMax = (DerivQ.z * 0.5f + Q.z) / (DerivK * 0.5f + K);
        PrevZMaxEst = RayZMax;        
        
        if(RayZMin > RayZMax)
        {
            Swap(RayZMin, RayZMax);
        }

        SceneZMax = LinearDepth(SAMPLE_DEPTH_FUNC(HitPixel), DepthProjection);
    }

    Q.xy += DerivQ.xy * StepCount;
    HitPoint = Q * (1.0f / K);

    return DepthIntersection(SceneZMax, RayZMin, RayZMax, Thickness);
}