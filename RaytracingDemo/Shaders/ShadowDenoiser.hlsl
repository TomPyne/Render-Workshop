struct ConstantData
{
    float4x4 CamToWorld;
    float4x4 PrevCamToWorld;

    float4x4 ClipToPrevClip;

    uint DepthTextureIndex;
    uint PrevDepthTextureIndex;
    uint ShadowTextureIndex;
    uint PrevShadowTextureIndex;

    uint ConfidenceTextureIndex; // Store
    uint PrevConfidenceTextureIndex;
    uint VelocityTextureIndex;
    uint DebugTextureIndex;

    float2 ViewportSizeRcp;
    uint2 ViewportSize;
};

ConstantBuffer<ConstantData> c_G : register(b1);

Texture2D<float> t_tex2d_f1[8192] : register(t0, space0); // Depth and shadow textures
Texture2D<float2> t_tex2d_f2[8192] : register(t0, space1); // Velocity
RWTexture2D<float> u_tex2d_f1[8192] : register(u0, space0);

SamplerState ClampedSampler : register(s1);

float3 GetWorldPosFromScreen(float4x4 CamToWorld, float2 ScreenPos, float Depth)
{
    float4 ProjectedPos = float4(ScreenPos, Depth,  1.0f);
    float4 Unprojected = mul(CamToWorld, ProjectedPos);
    return Unprojected.xyz / Unprojected.w;
}

[NumThreads(8, 8, 1)]
void main(uint3 DispatchThreadId : SV_DispatchThreadID)
{
    if(any(DispatchThreadId.xy > c_G.ViewportSize))
        return;

    float2 Pixel = float2(DispatchThreadId.xy) + 0.5;
    float2 NDC = (Pixel * c_G.ViewportSizeRcp) * 2.0f - 1.0f;
    float2 UV = Pixel * c_G.ViewportSizeRcp;

    // Load current sample
    float CurrentDepth = t_tex2d_f1[c_G.DepthTextureIndex][DispatchThreadId.xy].r;
    float CurrentShadow = u_tex2d_f1[c_G.ShadowTextureIndex][DispatchThreadId.xy];
    

    // Reconstruct previous sample
    float2 Velocity = t_tex2d_f2[c_G.VelocityTextureIndex][DispatchThreadId.xy].rg;
    Velocity.y *= -1.0f;
    float2 PrevNDC = NDC - Velocity;
    
    float3 CurrentWorldPosition = GetWorldPosFromScreen(c_G.CamToWorld, UV, CurrentDepth);
    
    float2 ReconstructedUv = (PrevNDC * 0.5f) + 0.5f;
    float2 ReconstructedScreenPos = ReconstructedUv *  c_G.ViewportSize;

    int2 ReconstructedPixel = int2(ReconstructedScreenPos);
    float DebugDepth = 0.0f;
    float Confidence = 0.0f;
    if(all(ReconstructedPixel >= 0) && all(ReconstructedPixel < c_G.ViewportSize))
    {
        float ReconstructedDepth = t_tex2d_f1[c_G.PrevDepthTextureIndex][ReconstructedPixel].r;
        float ReconstructedShadow = t_tex2d_f1[c_G.PrevShadowTextureIndex][ReconstructedPixel].r;
        float3 ReconstructedPosition = GetWorldPosFromScreen(c_G.PrevCamToWorld, ReconstructedUv, ReconstructedDepth);

        DebugDepth = ReconstructedDepth;
        // Compare world pos
        float3 Offset = CurrentWorldPosition - ReconstructedPosition;

        if(dot(Offset, Offset) < 0.1f)
        {
            Confidence = t_tex2d_f1[c_G.PrevConfidenceTextureIndex][ReconstructedPixel];

            Confidence = saturate(Confidence + ((0.99f - Confidence) * 0.1f));
        }
        else
        {
            Confidence = 0.0f; // Disoccluded
        }

        // Accumulate if similar
        u_tex2d_f1[c_G.ShadowTextureIndex][DispatchThreadId.xy] = lerp(CurrentShadow, ReconstructedShadow, Confidence);
    }

    //u_tex2d_f1[c_G.DebugTextureIndex][DispatchThreadId.xy] = Offset.r;

    // Output confidence, if similar then we inch towards 1, if unsimilar we reset to 0
    u_tex2d_f1[c_G.ConfidenceTextureIndex][DispatchThreadId.xy] = Confidence;
}