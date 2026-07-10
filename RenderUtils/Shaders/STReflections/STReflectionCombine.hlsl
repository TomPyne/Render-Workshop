#include "../Util/Macros.h"

struct Uniforms
{
    float4x4 InverseViewProjection;

    uint SceneColorTextureIndex;
    uint SceneDepthTextureIndex;
    uint SceneNormalTextureIndex;
    uint SceneRoughnessMetallicTextureIndex;

    uint ReflectionTextureIndex;
    float3 CamPosition;

    uint2 ScreenDim;
    float2 ScreenDimRcp;
};

ConstantBuffer<Uniforms> c_G : register(MAKE_CBV_SLOT(CBV_SLOT));

Texture2D<float4> t_tex2d_f4[8192] : register(t0, space0);
Texture2D<float2> t_tex2d_f2[8192] : register(t0, space1);
Texture2D<float> t_tex2d_f1[8192] : register(t0, space2);
RWTexture2D<float4> u_tex2d_f4[8192] : register(u0, space0);

// TODO: converting to view space means we dont need to pass the cam position and can use our shared library functions
float3 GetWorldPosFromScreen(float4x4 CamToWorld, float2 ScreenPos, float Depth)
{
    ScreenPos.y = -ScreenPos.y;
    float4 ProjectedPos = float4(ScreenPos, Depth,  1.0f);
    float4 Unprojected = mul(CamToWorld, ProjectedPos);
    return Unprojected.xyz / Unprojected.w;
}

[NumThreads(8, 8, 1)]
void main(uint3 DispatchThreadId : SV_DispatchThreadID)
{
    if(any(DispatchThreadId.xy >= c_G.ScreenDim))
        return;

    float2 UV = (float2(DispatchThreadId.xy) + 0.5f) * c_G.ScreenDimRcp;

    float4 SSR = t_tex2d_f4[c_G.ReflectionTextureIndex][DispatchThreadId.xy];
    
    float Depth = t_tex2d_f1[c_G.SceneDepthTextureIndex][DispatchThreadId.xy];

    float2 NDC = UV * 2.0f - 1.0f;
    float3 Position = GetWorldPosFromScreen(c_G.InverseViewProjection, NDC, Depth);
    float3 V = normalize(c_G.CamPosition - Position);

    float3 N = normalize(t_tex2d_f4[c_G.SceneNormalTextureIndex][DispatchThreadId.xy].xyz);

    float Fresnel = pow(1.0 - saturate(dot(N, V)), 5.0);

    float2 RoughnessMetallic = t_tex2d_f2[c_G.SceneRoughnessMetallicTextureIndex][DispatchThreadId.xy];
    float Roughness = max(RoughnessMetallic.r, 0.01f);  

    float RoughnessFade = saturate(1.0f - Roughness * Roughness);
    
    float SSRMask = SSR.a;
    SSRMask *= RoughnessFade;
    SSRMask *= Fresnel;

    float4 OrigColor = u_tex2d_f4[c_G.SceneColorTextureIndex][DispatchThreadId.xy];

    u_tex2d_f4[c_G.SceneColorTextureIndex][DispatchThreadId.xy] = float4(lerp(OrigColor.rgb, SSR.rgb, SSRMask), OrigColor.a);
}