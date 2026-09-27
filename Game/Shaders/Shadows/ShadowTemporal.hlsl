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
    float __Pad;
};

ConstantBuffer<Uniforms_s> c_Uniforms : register(b0);

Texture2D<float> t_tex2d_f1[8192] : register(t1, space0); // Depth, current shadow, linear depth history
Texture2D<float2> t_tex2d_f2[8192] : register(t1, space1); // Shadow + confidence history
Texture2D<float4> t_tex2d_f4[8192] : register(t1, space2); // Velocity
RWTexture2D<float> u_tex2d_f1[8192] : register(u0, space0); // Linear depth out
RWTexture2D<float2> u_tex2d_f2[8192] : register(u0, space1); // Shadow + confidence out

SamplerState ClampedSampler : register(s1);

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
        u_tex2d_f2[c_Uniforms.OutShadowTexture][PixelCoord] = float2(1.0f, 0.0f);
        u_tex2d_f1[c_Uniforms.OutLinearDepthTexture][PixelCoord] = 0.0f;
        return;
    }

    const float2 UV = (float2(PixelCoord) + 0.5f) * c_Uniforms.InvViewportSize;
    const float2 ScreenPos = float2(UV.x, 1.0f - UV.y) * 2.0f - 1.0f;

    // The unprojected w is 1 / clip w, and clip w is view depth
    const float4 Unprojected = mul(c_Uniforms.CamToWorld, float4(ScreenPos, SceneDepth, 1.0f));
    const float LinearDepth = 1.0f / Unprojected.w;

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

    float2 Result = float2(CurrentShadow, 0.0f);
    if (HistoryAccepted)
    {
        const float2 History = t_tex2d_f2[c_Uniforms.HistoryShadowTexture].SampleLevel(ClampedSampler, PrevUV, 0);

        const float Confidence = History.y + (c_Uniforms.MaxConfidence - History.y) * c_Uniforms.ConfidenceRate;
        Result = float2(lerp(CurrentShadow, History.x, Confidence), Confidence);
    }

    u_tex2d_f2[c_Uniforms.OutShadowTexture][PixelCoord] = Result;
    u_tex2d_f1[c_Uniforms.OutLinearDepthTexture][PixelCoord] = LinearDepth;
}

#endif // #ifdef _CS
