struct Uniforms
{
    uint InputTextureIndex;
    uint OutputTextureIndex;
    uint2 Dim;

    float2 DimRcp;
    float2 SourceDimRcp;

    float Threshold;
    float Knee;
    float2 __Pad0;
};

ConstantBuffer<Uniforms> c_G : register(b0);

Texture2D<float4> t_tex2d_f4[8192] : register(t1, space0);
RWTexture2D<float4> u_tex2d_f4[8192] : register(u0, space0);

SamplerState ClampedSampler : register(s1);

float RGBToLuminance(float3 RGB)
{
    return dot(float3(0.2126f, 0.7152f, 0.0722f), RGB);
}

// Can improve this using load ops instead of sample
float3 BlurSample(float2 UV)
{
    return t_tex2d_f4[c_G.InputTextureIndex].SampleLevel(ClampedSampler, UV, 0).rgb;
}

// Soft knee: zero below Threshold - Knee, quadratic ramp across the knee, linear (Luma - Threshold) above it.
float3 ApplyThreshold(float3 Color, float Threshold, float Knee)
{
    float Luma = RGBToLuminance(Color);
    float Soft = clamp(Luma - Threshold + Knee, 0.0f, 2.0f * Knee);
    Soft = (Soft * Soft) / (4.0f * Knee + 1e-5f);
    float Contribution = max(Soft, Luma - Threshold) / max(Luma, 1e-5f);
    return Color * Contribution;
}

[numthreads(8, 8, 1)]
void main(uint3 DispatchThreadId : SV_DispatchThreadID)
{
    if(any(DispatchThreadId.xy >= c_G.Dim))
        return;

    float2 UV = (float2(DispatchThreadId.xy) + 0.5f) * c_G.DimRcp;

    float X = c_G.SourceDimRcp.x;
    float Y = c_G.SourceDimRcp.y;
    float X2 = 2.0f * X;
    float Y2 = 2.0f * Y;

    // This whole thing stinks of poor occupancy.
    // Try running just 4x float3s at a time
    float3 A = BlurSample(float2(UV.x - X2, UV.y + Y2));
    float3 B = BlurSample(float2(UV.x,      UV.y + Y2));
    float3 C = BlurSample(float2(UV.x + X2, UV.y + Y2));
                        
    float3 D = BlurSample(float2(UV.x - X2, UV.y));
    float3 E = BlurSample(float2(UV.x,      UV.y));
    float3 F = BlurSample(float2(UV.x + X2, UV.y));

    float3 G = BlurSample(float2(UV.x - X2, UV.y - Y2));
    float3 H = BlurSample(float2(UV.x,      UV.y - Y2));
    float3 I = BlurSample(float2(UV.x + X2, UV.y - Y2));

    float3 J = BlurSample(float2(UV.x - X, UV.y + Y));
    float3 K = BlurSample(float2(UV.x + X, UV.y + Y));
    float3 L = BlurSample(float2(UV.x - X, UV.y - Y));
    float3 M = BlurSample(float2(UV.x + X, UV.y - Y));

    // The 13 taps form five overlapping 2x2 boxes: the inner box carries half the weight, the four corner boxes an eighth each.
    // Summed with plain weights this is identical to the per-tap kernel.
    float3 Boxes[5] =
    {
        (J + K + L + M) * 0.25f,
        (A + B + D + E) * 0.25f,
        (B + C + E + F) * 0.25f,
        (D + E + G + H) * 0.25f,
        (E + F + H + I) * 0.25f,
    };
    float BoxWeights[5] = { 0.5f, 0.125f, 0.125f, 0.125f, 0.125f };

    float3 Downsampled = 0.0f;
    float WeightSum = 0.0f;
    [unroll]
    for (uint BoxIt = 0; BoxIt < 5; ++BoxIt)
    {
        // Karis average: weight each box by 1 / (1 + luma) so a single very bright texel can't dominate the result.
        BoxWeights[BoxIt] *= 1.0f / (1.0f + RGBToLuminance(Boxes[BoxIt]));
        Downsampled += Boxes[BoxIt] * BoxWeights[BoxIt];
        WeightSum += BoxWeights[BoxIt];
    }
    Downsampled /= WeightSum;

    if (c_G.Threshold > 0.0f)
    {
        Downsampled = ApplyThreshold(Downsampled, c_G.Threshold, c_G.Knee);
    }

    u_tex2d_f4[c_G.OutputTextureIndex][DispatchThreadId.xy] = float4(Downsampled, 1);
}