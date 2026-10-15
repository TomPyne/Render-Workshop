#include "../Samplers.h"
#include "CubeShared.h"

struct Uniforms_s
{
    uint EquirectTextureIndex;
    uint OutputTextureIndex;
    uint Size;
    float Yaw;

    uint FurnaceTest;
    float3 __Pad;
};

ConstantBuffer<Uniforms_s> c_G : register(b0);

Texture2D<float4> t_tex2d_f4[8192] : register(t1, space0);
RWTexture2DArray<float4> u_tex2darr_f4[8192] : register(u0, space0);

// Equirect u starts at -Z and runs through +X, v runs from +Y down to -Y
float3 SampleEquirect(Texture2D<float4> Equirect, float3 Dir, float VTexel)
{
    const float U = atan2(Dir.x, Dir.z) / (2.0f * PI) + 0.5f;
    // The wrap sampler handles the u seam, clamping v stops the poles filtering into each other
    const float V = clamp(acos(clamp(Dir.y, -1.0f, 1.0f)) / PI, 0.5f * VTexel, 1.0f - 0.5f * VTexel);
    return Equirect.SampleLevel(SharedWrappedSampler, float2(U, V), 0.0f).rgb;
}

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (any(DTid.xy >= c_G.Size))
    {
        return;
    }

    float3 Radiance = 1.0f;

    if (!c_G.FurnaceTest)
    {
        Texture2D<float4> Equirect = t_tex2d_f4[c_G.EquirectTextureIndex];

        uint Width, Height;
        Equirect.GetDimensions(Width, Height);
        const float VTexel = 1.0f / Height;

        // Rotating the environment by +Yaw is rotating the lookup by -Yaw
        float SinYaw, CosYaw;
        sincos(c_G.Yaw, SinYaw, CosYaw);

        // 2x2 supersampled, the equirect is often denser than the cube and has no mips
        Radiance = 0.0f;
        for (uint SampleIt = 0; SampleIt < 4; SampleIt++)
        {
            const float2 Offset = float2(SampleIt & 1, SampleIt >> 1) * 0.5f + 0.25f;
            const float2 UV = (DTid.xy + Offset) / c_G.Size;
            const float3 Dir = normalize(CubeFaceDirection(DTid.z, UV));
            const float3 Rotated = float3(Dir.x * CosYaw - Dir.z * SinYaw, Dir.y, Dir.x * SinYaw + Dir.z * CosYaw);
            Radiance += SampleEquirect(Equirect, Rotated, VTexel);
        }
        Radiance *= 0.25f;
    }

    u_tex2darr_f4[c_G.OutputTextureIndex][DTid] = float4(Radiance, 1.0f);
}
