struct ConstantData
{
    uint ConfidenceTextureIndex;
    uint ShadowTextureIndex;
    uint ShadowHistoryTextureIndex;
    uint VelocityTextureIndex;

    float2 ViewportSizeRcp;
    uint2 ViewportSize;
};

ConstantBuffer<ConstantData> c_G : register(b1);

Texture2D<float> t_tex2d_f1[8192] : register(t0, space0); // Confidence and shadow textures
Texture2D<float2> t_tex2d_f2[8192] : register(t0, space1); // Velocity
RWTexture2D<float> u_tex2d_f1[8192] : register(u0, space0); // Output shadow

SamplerState ClampedSampler : register(s1);

[NumThreads(8, 8, 1)]
void main(uint3 DispatchThreadId : SV_DispatchThreadID)
{
    if(any(DispatchThreadId.xy > c_G.ViewportSize))
        return;

    float Shadow = u_tex2d_f1[c_G.ShadowTextureIndex][DispatchThreadId.xy].r;
    float Confidence = t_tex2d_f1[c_G.ConfidenceTextureIndex][DispatchThreadId.xy].r;

    float2 Pixel = float2(DispatchThreadId.xy) + 0.5;
    float2 NDC = (Pixel * c_G.ViewportSizeRcp) * 2.0f - 1.0f;

    // Reconstruct previous sample
    float2 Velocity = t_tex2d_f2[c_G.VelocityTextureIndex][DispatchThreadId.xy].rg;
    Velocity.y *= -1.0f;
    float2 PrevNDC = NDC - Velocity;
    
    float2 ReconstructedUv = (PrevNDC * 0.5f) + 0.5f;
    float2 ReconstructedScreenPos = ReconstructedUv *  c_G.ViewportSize;

    int2 ReconstructedPixel = int2(ReconstructedScreenPos - 0.5f);

    float ReconstructedShadow = 1.0f;
    if(all(ReconstructedPixel >= 0) && all(ReconstructedPixel < c_G.ViewportSize))
    {
        ReconstructedShadow = t_tex2d_f1[c_G.ShadowHistoryTextureIndex].SampleLevel(ClampedSampler, ReconstructedUv, 0).r;
    }

    // Accumulate
    u_tex2d_f1[c_G.ShadowTextureIndex][DispatchThreadId.xy] = lerp(Shadow, ReconstructedShadow, Confidence);
}