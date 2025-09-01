struct ConstantData
{
    float4x4 CamToWorld;
    float4x4 PrevCamToWorld;

    uint32_t DepthTextureIndex;
    uint32_t PrevDepthTextureIndex;
    uint32_t ShadowTextureIndex;
    uint32_t PrevShadowTextureIndex;

    uint32_t ConfidenceTextureIndex; // Store
    uint32_t VelocityTextureIndex;
    float2 ViewportSizeRcp;

    uint2 ViewportSize;
    float2 __pad0;
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
    float2 UV = Pixel * c_G.ViewportSizeRcp;

    // Load current sample
    float CurrentDepth = t_tex2d_f1[c_G.DepthTextureIndex].SampleLevel(ClampedSampler, UV, 0u).r;
    float CurrentShadow = u_tex2d_f1[c_G.ShadowTextureIndex][DispatchThreadId.xy];
    float3 CurrentWorldPosition = GetWorldPosFromScreen(c_G.CamToWorld, UV, CurrentDepth);   

    // Reconstruct previous sample
    float2 Velocity = t_tex2d_f2[c_G.VelocityTextureIndex].SampleLevel(ClampedSampler, UV, 0u).rg;
    float2 ReconstructedSVPos = Pixel + Velocity;
    float2 ReconstructedUv = ReconstructedSVPos * c_G.ViewportSizeRcp;

    float ReconstructedDepth = t_tex2d_f1[c_G.PrevDepthTextureIndex].SampleLevel(ClampedSampler, ReconstructedUv, 0u).r;   
    float ReconstructedShadow = t_tex2d_f1[c_G.PrevShadowTextureIndex].SampleLevel(ClampedSampler, ReconstructedUv, 0u).r;         
    float3 ReconstructedPosition = GetWorldPosFromScreen(c_G.PrevCamToWorld, ReconstructedUv, ReconstructedDepth);

    // Compare world pos
    float3 Offset = CurrentWorldPosition - ReconstructedPosition;
    float Confidence = u_tex2d_f1[c_G.ConfidenceTextureIndex][DispatchThreadId.xy];

    if(dot(Offset, Offset) > 1.0f)
    {
        Confidence = 0.0f; // Disoccluded
    }   

    // Accumulate if similar
    u_tex2d_f1[c_G.ShadowTextureIndex][DispatchThreadId.xy] = lerp(CurrentShadow, ReconstructedShadow, Confidence);

    // Output confidence, if similar then we inch towards 1, if unsimilar we reset to 0
    u_tex2d_f1[c_G.ConfidenceTextureIndex][DispatchThreadId.xy] = lerp(Confidence, 1.0f, 0.01f);
}