#ifdef _PS

#include "Samplers.h"
#include "ScreenPassShared.h"
#include "View.h"

struct DeferredData_s
{
    float4x4 InvViewProjection;

    float3 LightDirection;
    uint SceneColorMetallicTextureIndex;

    float3 LightRadiance;
    uint SceneNormalRoughnessTextureIndex;

    float3 AmbientColor;
    uint SceneEmissiveSpecularTextureIndex;

    uint SceneDepthTextureIndex;
    float3 __Pad;
};

ConstantBuffer<ViewUniforms_s> c_View : register(b1);
ConstantBuffer<DeferredData_s> c_Deferred : register(b2);
Texture2D<float4> t_tex2d_f4[8192] : register(t0, space0);

static const float PI = 3.14159265f;

float D_GGX(float NoH, float A2)
{
    const float D = (NoH * A2 - NoH) * NoH + 1.0f;
    return A2 / (PI * D * D);
}

// Height-correlated Smith, with the 1 / (4 NoL NoV) denominator folded in
float V_SmithGGXCorrelated(float NoV, float NoL, float A2)
{
    const float GGXV = NoL * sqrt(NoV * NoV * (1.0f - A2) + A2);
    const float GGXL = NoV * sqrt(NoL * NoL * (1.0f - A2) + A2);
    return 0.5f / (GGXV + GGXL);
}

float3 F_Schlick(float3 F0, float VoH)
{
    const float Fc = pow(1.0f - VoH, 5.0f);
    return Fc + F0 * (1.0f - Fc);
}

// Karis, "Physically Based Shading on Mobile"
float3 EnvBRDFApprox(float3 F0, float Roughness, float NoV)
{
    const float4 C0 = float4(-1.0f, -0.0275f, -0.572f, 0.022f);
    const float4 C1 = float4(1.0f, 0.0425f, 1.04f, -0.04f);
    const float4 R = Roughness * C0 + C1;
    const float A004 = min(R.x * R.x, exp2(-9.28f * NoV)) * R.x + R.y;
    const float2 AB = float2(-1.04f, 1.04f) * A004 + R.zw;
    return F0 * AB.x + AB.y;
}

float3 ReconstructWorldPosition(float2 UV, float Depth)
{
    const float4 NDC = float4(UV.x * 2.0f - 1.0f, 1.0f - UV.y * 2.0f, Depth, 1.0f);
    const float4 World = mul(c_Deferred.InvViewProjection, NDC);
    return World.xyz / World.w;
}

void main(in Interpolants_s Input, out float4 Output : SV_TARGET)
{
    const int3 Pixel = int3(Input.SVPosition.xy, 0);

    const float Depth = t_tex2d_f4[c_Deferred.SceneDepthTextureIndex].Load(Pixel).r;
    if (Depth >= 1.0f)
    {
        Output = float4(c_Deferred.AmbientColor, 0.0f);
        return;
    }

    const float4 AlbedoMetallic = t_tex2d_f4[c_Deferred.SceneColorMetallicTextureIndex].Load(Pixel);
    const float4 NormalRoughness = t_tex2d_f4[c_Deferred.SceneNormalRoughnessTextureIndex].Load(Pixel);
    const float4 EmissiveSpecular = t_tex2d_f4[c_Deferred.SceneEmissiveSpecularTextureIndex].Load(Pixel);

    const float3 Albedo = AlbedoMetallic.rgb;
    const float Metallic = saturate(AlbedoMetallic.a);
    const float Roughness = saturate(NormalRoughness.a);
    const float Specular = saturate(EmissiveSpecular.a);
    const float3 Emissive = EmissiveSpecular.rgb;

    const float3 DiffuseColor = Albedo * (1.0f - Metallic);
    const float3 F0 = lerp(0.08f * Specular.xxx, Albedo, Metallic);

    const float Alpha = max(Roughness * Roughness, 0.002f);
    const float A2 = Alpha * Alpha;

    const float3 WorldPos = ReconstructWorldPosition(Input.UV, Depth);
    const float3 N = normalize(NormalRoughness.xyz);
    const float3 V = normalize(c_View.CamPos - WorldPos);
    const float3 L = c_Deferred.LightDirection;
    const float3 H = normalize(V + L);

    const float NoV = max(dot(N, V), 1e-4f);
    const float NoL = saturate(dot(N, L));
    const float NoH = saturate(dot(N, H));
    const float VoH = saturate(dot(V, H));

    const float3 DiffuseBRDF = DiffuseColor / PI;
    const float3 SpecularBRDF = D_GGX(NoH, A2) * V_SmithGGXCorrelated(NoV, NoL, A2) * F_Schlick(F0, VoH);
    const float3 Direct = (DiffuseBRDF + SpecularBRDF) * c_Deferred.LightRadiance * NoL;

    const float3 Ambient = c_Deferred.AmbientColor * (DiffuseColor + EnvBRDFApprox(F0, Roughness, NoV));

    Output = float4(Direct + Ambient + Emissive, 0.0f);
}

#endif // #ifdef _PS
