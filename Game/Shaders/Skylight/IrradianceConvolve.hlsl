#include "../Samplers.h"
#include "CubeShared.h"

struct Uniforms_s
{
    uint SourceTextureIndex;
    uint OutputTextureIndex;
    uint Size;
    uint SourceSize;

    float SourceMip;
    float3 __Pad;
};

ConstantBuffer<Uniforms_s> c_G : register(b0);

TextureCube<float4> t_texcube_f4[8192] : register(t1, space0);
RWTexture2DArray<float4> u_tex2darr_f4[8192] : register(u0, space0);

float CubeAreaElement(float X, float Y)
{
    return atan2(X * Y, sqrt(X * X + Y * Y + 1.0f));
}

// Exact, the same for every face
float CubeTexelSolidAngle(uint2 Texel, float InvSize)
{
    const float2 P0 = Texel * 2.0f * InvSize - 1.0f;
    const float2 P1 = P0 + 2.0f * InvSize;
    return CubeAreaElement(P0.x, P0.y) - CubeAreaElement(P0.x, P1.y) - CubeAreaElement(P1.x, P0.y) + CubeAreaElement(P1.x, P1.y);
}

// Brute force cosine convolution of a low source mip. Normalizing by the summed cosine weighted solid angle (pi over the
// hemisphere) rather than by pi gives E / pi, the mean radiance that multiplies the diffuse albedo directly
[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (any(DTid.xy >= c_G.Size))
    {
        return;
    }

    TextureCube<float4> Source = t_texcube_f4[c_G.SourceTextureIndex];

    const float3 N = normalize(CubeFaceDirection(DTid.z, (DTid.xy + 0.5f) / c_G.Size));
    const float InvSourceSize = 1.0f / c_G.SourceSize;

    float3 Sum = 0.0f;
    float WeightSum = 0.0f;

    for (uint Y = 0; Y < c_G.SourceSize; Y++)
    {
        for (uint X = 0; X < c_G.SourceSize; X++)
        {
            const float SolidAngle = CubeTexelSolidAngle(uint2(X, Y), InvSourceSize);
            const float2 UV = (float2(X, Y) + 0.5f) * InvSourceSize;

            for (uint Face = 0; Face < 6; Face++)
            {
                const float3 L = normalize(CubeFaceDirection(Face, UV));
                const float NoL = dot(N, L);
                if (NoL <= 0.0f)
                {
                    continue;
                }

                // Texel centre, so the linear filter returns the texel as is
                const float Weight = NoL * SolidAngle;
                Sum += Source.SampleLevel(SharedClampedSampler, L, c_G.SourceMip).rgb * Weight;
                WeightSum += Weight;
            }
        }
    }

    u_tex2darr_f4[c_G.OutputTextureIndex][DTid] = float4(Sum / max(WeightSum, 1e-6f), 1.0f);
}
