#include "../Samplers.h"
#include "CubeShared.h"
#include "../DrawConstants.h"

struct Uniforms_s
{
    uint SourceTextureIndex;
    uint OutputTextureIndex;
    uint Size;
    uint SampleCount;

    float Roughness;
    // Solid angle of a source mip 0 texel
    float SourceTexelSolidAngle;
    float SourceMaxMip;
    // Source mip with the same texel size as the output's mip 0
    float CopyMip;
};

DRAW_CONSTANTS(Uniforms_s, c_G);

TextureCube<float4> t_texcube_f4[8192] : register(t1, space0);
RWTexture2DArray<float4> u_tex2darr_f4[8192] : register(u0, space0);

float2 Hammersley(uint Index, uint Count)
{
    return float2(float(Index) / float(Count), float(reversebits(Index)) * 2.3283064365386963e-10f);
}

float3 ImportanceSampleGGX(float2 Xi, float A2, float3 N)
{
    const float Phi = 2.0f * PI * Xi.x;
    const float CosTheta = sqrt((1.0f - Xi.y) / (1.0f + (A2 - 1.0f) * Xi.y));
    const float SinTheta = sqrt(1.0f - CosTheta * CosTheta);

    const float3 Up = abs(N.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
    const float3 T = normalize(cross(Up, N));
    const float3 B = cross(N, T);

    return normalize(T * (SinTheta * cos(Phi)) + B * (SinTheta * sin(Phi)) + N * CosTheta);
}

float D_GGX(float NoH, float A2)
{
    const float D = (NoH * A2 - NoH) * NoH + 1.0f;
    return A2 / (PI * D * D);
}

// Split sum prefilter with N = V = R. Each sample reads a source mip matching its share of the lobe's solid angle
// (filtered importance sampling, GPU Gems 3 ch. 20), which keeps small bright sources like the sun from sparkling
[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (any(DTid.xy >= c_G.Size))
    {
        return;
    }

    TextureCube<float4> Source = t_texcube_f4[c_G.SourceTextureIndex];

    const float3 N = normalize(CubeFaceDirection(DTid.z, (DTid.xy + 0.5f) / c_G.Size));

    if (c_G.Roughness <= 0.0f)
    {
        u_tex2darr_f4[c_G.OutputTextureIndex][DTid] = float4(Source.SampleLevel(SharedClampedSampler, N, c_G.CopyMip).rgb, 1.0f);
        return;
    }

    const float Alpha = c_G.Roughness * c_G.Roughness;
    const float A2 = Alpha * Alpha;

    float3 Sum = 0.0f;
    float WeightSum = 0.0f;

    for (uint SampleIt = 0; SampleIt < c_G.SampleCount; SampleIt++)
    {
        const float3 H = ImportanceSampleGGX(Hammersley(SampleIt, c_G.SampleCount), A2, N);
        const float NoH = saturate(dot(N, H));
        const float3 L = 2.0f * NoH * H - N;

        const float NoL = dot(N, L);
        if (NoL <= 0.0f)
        {
            continue;
        }

        // pdf = D * NoH / (4 * VoH), which is D / 4 with N = V
        const float Pdf = D_GGX(NoH, A2) * 0.25f;
        const float SampleSolidAngle = 1.0f / (c_G.SampleCount * Pdf + 1e-6f);
        const float Lod = clamp(0.5f * log2(SampleSolidAngle / c_G.SourceTexelSolidAngle) + 1.0f, 0.0f, c_G.SourceMaxMip);

        Sum += Source.SampleLevel(SharedClampedSampler, L, Lod).rgb * NoL;
        WeightSum += NoL;
    }

    u_tex2darr_f4[c_G.OutputTextureIndex][DTid] = float4(Sum / max(WeightSum, 1e-6f), 1.0f);
}
