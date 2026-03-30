#ifdef _PS

#include "DeferredCommon.h"
#include "ScreenPassShared.h"

ConstantBuffer<DeferredData> c_Deferred : register(b1);
Texture2D<float4> t_tex2d_f4[8192] : register(t0, space0);
Texture2D<float2> t_tex2d_f2[8192] : register(t0, space1);
Texture2D<float> t_tex2d_f1[8192] : register(t0, space2);
SamplerState ClampedSampler : register(s1);

#define PI 3.141592653589793f

float Lambert()
{
    return 1.0f / PI;
}

float GGX(float NoH, float Roughness)
{
    float A2 = Roughness * Roughness;
    float F = (NoH * A2 - NoH) * NoH + 1.0f;
    return A2 / (PI * F * F);
}

float GeometrySchlickGGX(float NoV, float K)
{
    float Denom = NoV * (1.0 - K) + K;
	
    return NoV / Denom;
}

float SmithGGXCorrelated(float NoV, float NoL, float A)
{
    float A2 = A * A;
    float GGXL = NoV * sqrt((-NoL * A2 + NoL) * NoL + A2);
    float GGXV = NoL * sqrt((-NoV * A2 + NoV) * NoV + A2);
    return 0.5f / (GGXV + GGXL);
}

float3 Schlick(float U, float3 F0)
{
    static const float3 F90 = float3(1.0f, 1.0f, 1.0f);
    return F0 + (F90 - F0) * pow(1.0f - U, 5.0f);
}

struct PS_OUTPUT
{
    float4 Color : SV_TARGET0;
};

float3 GetWorldPos(float4x4 CamToWorld, float2 UV, float Depth)
{
    float x = UV.x * 2 - 1;
    float y = UV.y * 2 - 1;
    float4 ProjectedPos = float4(x, y, Depth,  1.0f);

    float4 Unprojected = mul(CamToWorld, ProjectedPos);
    return Unprojected.xyz / Unprojected.w;
}

void main(in PS_INPUT Input, out PS_OUTPUT Output)
{
    float3 Color = t_tex2d_f4[c_Deferred.SceneColorTextureIndex].SampleLevel(ClampedSampler, Input.UV, 0u).rgb;
    float Depth = t_tex2d_f1[c_Deferred.DepthTextureIndex].SampleLevel(ClampedSampler, Input.UV, 0u).r;
    if(Depth >= 1.0f)
    {
        Output.Color = float4(Color, 1);
        return;
    }

    float3 Normal = (t_tex2d_f4[c_Deferred.SceneNormalTextureIndex].SampleLevel(ClampedSampler, Input.UV, 0u).rgb);
    Normal.x *= -1.0f;
    
    float2 RoughnessMetallic = t_tex2d_f2[c_Deferred.SceneRoughnessMetallicTextureIndex].SampleLevel(ClampedSampler, Input.UV, 0u).rg * float2(0.9, 1.0);
    
    float3 Position = GetWorldPosFromScreen(c_Deferred.CamToWorld, Input.SVPosition.xy * c_Deferred.ViewportSizeRcp, Depth);

    float Shadow = t_tex2d_f1[c_Deferred.ShadowTextureIndex].SampleLevel(ClampedSampler, Input.UV, 0u).r;

    float Roughness = RoughnessMetallic.r;
    float Metallic = RoughnessMetallic.g;

    float3 L = c_Deferred.SunDirection;
    float3 V = normalize(c_Deferred.CamPosition - Position);
    float3 H = normalize(V + L);

    float NoV = abs(dot(Normal, V)) + 0.00001f;
    float NoL = saturate(dot(Normal, L));
    float NoH = saturate(dot(Normal, H));
    float LoH = saturate(dot(L, H));
    
    float3 Reflectance = 0.55f;
    float3 F0 = 0.16f * Reflectance * Reflectance * (1.0f - Metallic) + Color * Metallic;

    float D = GGX(NoH, Roughness);
    float3 F = Schlick(LoH, F0);
    float Vis = SmithGGXCorrelated(NoV, NoL, Roughness);   

    float3 SpecularTerm = (D * Vis) * F * 4; // TODO energy loss compensation

    float3 Diffuse = (1.0f - Metallic) * Color;
    float3 DiffuseTerm = Diffuse * Lambert();

    float3 Lighting = (SpecularTerm + DiffuseTerm) * Shadow * 5 * NoL;
    
    Output.Color = float4(Lighting + Diffuse, 1.0f);
}
#endif