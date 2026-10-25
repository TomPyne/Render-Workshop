#include "../Samplers.h"
#include "HeightFogShared.h"
#include "../DrawConstants.h"

// Must match HeightFogUniforms_s in Fog.cpp
struct Uniforms_s
{
    float4x4 InvViewProjection;

    float3 CameraPosition;
    uint SceneColorTextureIndex;

    float3 SunDirection;
    uint SceneDepthTextureIndex;

    float3 SunInscatter;
    uint SkyPrefilterTextureIndex;

    float3 AmbientLuminance;
    uint SkyEnabled;

    float3 Albedo;
    float SkyIntensity;

    float Density;
    float HeightFalloff;
    float BaseHeight;
    float StartDistance;

    float MaxOpacity;
    float SunAnisotropy;
    float SkyPrefilterMaxMip;
    float SkyDistance;

    uint2 ScreenSize;
    float2 InvScreenSize;
};

DRAW_CONSTANTS(Uniforms_s, c_G);

Texture2D<float4> t_tex2d_f4[8192] : register(t1, space0);
TextureCube<float4> t_texcube_f4[8192] : register(t1, space2);
RWTexture2D<float4> u_tex2d_f4[8192] : register(u0, space0);

[numthreads(8, 8, 1)]
void main(uint3 DispatchThreadId : SV_DispatchThreadID)
{
    if (any(DispatchThreadId.xy >= c_G.ScreenSize))
        return;

    const float Depth = t_tex2d_f4[c_G.SceneDepthTextureIndex].Load(int3(DispatchThreadId.xy, 0)).r;

    const float2 UV = (float2(DispatchThreadId.xy) + 0.5f) * c_G.InvScreenSize;
    const float4 NDC = float4(UV.x * 2.0f - 1.0f, 1.0f - UV.y * 2.0f, min(Depth, 1.0f), 1.0f);
    const float4 World = mul(c_G.InvViewProjection, NDC);
    const float3 ToSurface = World.xyz / World.w - c_G.CameraPosition;
    const float3 Dir = normalize(ToSurface);

    // The sky sits at SkyDistance, and geometry is clamped to it so nothing is fogged more than the sky behind it
    const float Distance = Depth >= 1.0f ? c_G.SkyDistance : min(length(ToSurface), c_G.SkyDistance);
    const float3 WorldPos = c_G.CameraPosition + Dir * Distance;

    HeightFogParams_s Params;
    Params.Density = c_G.Density;
    Params.HeightFalloff = c_G.HeightFalloff;
    Params.BaseHeight = c_G.BaseHeight;
    Params.StartDistance = c_G.StartDistance;
    Params.MaxOpacity = c_G.MaxOpacity;

    const float Transmittance = HeightFogTransmittance(c_G.CameraPosition, WorldPos, Params);
    if (Transmittance >= 1.0f)
        return;

    // The blurriest prefilter mip along the view ray stands in for the sky light scattered towards the eye
    const float3 SkyInscatter = c_G.SkyEnabled
        ? c_G.SkyIntensity * t_texcube_f4[c_G.SkyPrefilterTextureIndex].SampleLevel(SharedClampedSampler, Dir, c_G.SkyPrefilterMaxMip).rgb
        : c_G.AmbientLuminance;

    const float3 SunInscatter = c_G.SunInscatter * HenyeyGreensteinPhase(c_G.SunAnisotropy, dot(Dir, c_G.SunDirection));

    const float3 Inscatter = c_G.Albedo * (SkyInscatter + SunInscatter);

    const float3 Color = u_tex2d_f4[c_G.SceneColorTextureIndex][DispatchThreadId.xy].rgb;
    u_tex2d_f4[c_G.SceneColorTextureIndex][DispatchThreadId.xy] = float4(Color * Transmittance + Inscatter * (1.0f - Transmittance), 1.0f);
}
