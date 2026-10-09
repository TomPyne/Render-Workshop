#ifdef _CS

struct Uniforms_s
{
    float4x4 CamToWorld;

    uint2 ViewportSize;
    float2 InvViewportSize;

    uint SceneDepthTexture;
    uint VelocityTexture;
    uint CurrentShadowTexture;
    uint HistoryShadowTexture;

    uint HistoryLinearDepthTexture;
    uint OutShadowTexture;
    uint OutLinearDepthTexture;
    uint HistoryValid;

    float MaxConfidence;
    float ConfidenceRate;
    float DepthTolerance;
    uint OutShadowVarianceTexture;

    uint SceneNormalTexture;
    float MinConfidenceForTemporalVariance;
    float2 __Pad;
};

ConstantBuffer<Uniforms_s> c_Uniforms : register(b0);

Texture2D<float> t_tex2d_f1[8192] : register(t1, space0); // Depth, current shadow, linear depth history
Texture2D<float4> t_tex2d_f4[8192] : register(t1, space1); // Velocity, normal, shadow moments + confidence history
RWTexture2D<float> u_tex2d_f1[8192] : register(u0, space0); // Linear depth out
RWTexture2D<float2> u_tex2d_f2[8192] : register(u0, space1); // Shadow + variance out
RWTexture2D<float4> u_tex2d_f4[8192] : register(u0, space2); // Shadow moments + confidence history out

SamplerState ClampedSampler : register(s1);

static const int kVarianceRadius = 2;
// About 25 degrees
static const float kVarianceMinNormalDot = 0.9f;

float GetLinearDepth(uint2 PixelCoord, float SceneDepth)
{
    const float2 UV = (float2(PixelCoord) + 0.5f) * c_Uniforms.InvViewportSize;
    const float2 ScreenPos = float2(UV.x, 1.0f - UV.y) * 2.0f - 1.0f;

    // The unprojected w is 1 / clip w, and clip w is view depth
    return 1.0f / dot(c_Uniforms.CamToWorld[3], float4(ScreenPos, SceneDepth, 1.0f));
}

// Variance of this frame's shadow over neighbours on the same surface, for pixels without enough history
float EstimateSpatialVariance(uint2 PixelCoord, float CentreLinearDepth, float3 CentreNormal)
{
    float Count = 0.0f;
    float Sum = 0.0f;
    float SumSq = 0.0f;

    for (int Y = -kVarianceRadius; Y <= kVarianceRadius; Y++)
    {
        for (int X = -kVarianceRadius; X <= kVarianceRadius; X++)
        {
            const int2 Coord = int2(PixelCoord) + int2(X, Y);
            if (any(Coord < 0) || any(Coord >= int2(c_Uniforms.ViewportSize)))
                continue;

            const float Depth = t_tex2d_f1[c_Uniforms.SceneDepthTexture][Coord];
            if (Depth >= 1.0f)
                continue;

            if (abs(GetLinearDepth(Coord, Depth) - CentreLinearDepth) > CentreLinearDepth * c_Uniforms.DepthTolerance)
                continue;

            const float3 Normal = normalize(t_tex2d_f4[c_Uniforms.SceneNormalTexture][Coord].xyz);
            if (dot(Normal, CentreNormal) < kVarianceMinNormalDot)
                continue;

            const float Shadow = t_tex2d_f1[c_Uniforms.CurrentShadowTexture][Coord];
            Count += 1.0f;
            Sum += Shadow;
            SumSq += Shadow * Shadow;
        }
    }

    const float Mean = Sum / Count;
    return max(0.0f, SumSq / Count - Mean * Mean);
}

[NumThreads(8, 8, 1)]
void main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    const uint2 PixelCoord = DispatchThreadID.xy;
    if (any(PixelCoord >= c_Uniforms.ViewportSize))
        return;

    const float SceneDepth = t_tex2d_f1[c_Uniforms.SceneDepthTexture][PixelCoord];

    // Sky, nothing to shadow or reproject
    if (SceneDepth >= 1.0f)
    {
        u_tex2d_f4[c_Uniforms.OutShadowTexture][PixelCoord] = float4(1.0f, 1.0f, 0.0f, 0.0f);
        u_tex2d_f2[c_Uniforms.OutShadowVarianceTexture][PixelCoord] = float2(1.0f, 0.0f);
        u_tex2d_f1[c_Uniforms.OutLinearDepthTexture][PixelCoord] = 0.0f;
        return;
    }

    const float2 UV = (float2(PixelCoord) + 0.5f) * c_Uniforms.InvViewportSize;
    const float LinearDepth = GetLinearDepth(PixelCoord, SceneDepth);

    const float CurrentShadow = t_tex2d_f1[c_Uniforms.CurrentShadowTexture][PixelCoord];

    // xy is CurrentUV - PrevUV, z is this surface's view depth last frame
    const float4 Velocity = t_tex2d_f4[c_Uniforms.VelocityTexture][PixelCoord];
    const float2 PrevUV = UV - Velocity.xy;
    const float ExpectedPrevDepth = Velocity.z;

    bool HistoryAccepted = c_Uniforms.HistoryValid != 0 && ExpectedPrevDepth > 0.0f && all(PrevUV >= 0.0f) && all(PrevUV < 1.0f);
    if (HistoryAccepted)
    {
        // Nearest texel, a bilinear depth would blend across silhouettes
        const uint2 PrevPixelCoord = min(uint2(PrevUV * c_Uniforms.ViewportSize), c_Uniforms.ViewportSize - 1);
        const float HistoryDepth = t_tex2d_f1[c_Uniforms.HistoryLinearDepthTexture][PrevPixelCoord];

        HistoryAccepted = abs(HistoryDepth - ExpectedPrevDepth) < ExpectedPrevDepth * c_Uniforms.DepthTolerance;
    }

    // Mean, second moment, confidence
    float3 Result = float3(CurrentShadow, CurrentShadow * CurrentShadow, 0.0f);
    if (HistoryAccepted)
    {
        const float4 History = t_tex2d_f4[c_Uniforms.HistoryShadowTexture].SampleLevel(ClampedSampler, PrevUV, 0);

        const float Confidence = History.z + (c_Uniforms.MaxConfidence - History.z) * c_Uniforms.ConfidenceRate;
        Result = float3(lerp(Result.xy, History.xy, Confidence), Confidence);
    }

    float Variance = max(0.0f, Result.y - Result.x * Result.x);
    if (Result.z < c_Uniforms.MinConfidenceForTemporalVariance)
    {
        const float3 Normal = normalize(t_tex2d_f4[c_Uniforms.SceneNormalTexture][PixelCoord].xyz);
        Variance = EstimateSpatialVariance(PixelCoord, LinearDepth, Normal);
    }

    u_tex2d_f4[c_Uniforms.OutShadowTexture][PixelCoord] = float4(Result, 0.0f);
    u_tex2d_f2[c_Uniforms.OutShadowVarianceTexture][PixelCoord] = float2(Result.x, Variance);
    u_tex2d_f1[c_Uniforms.OutLinearDepthTexture][PixelCoord] = LinearDepth;
}

#endif // #ifdef _CS
