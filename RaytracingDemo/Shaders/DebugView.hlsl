#ifdef _PS

#include "DeferredCommon.h"
#include "ScreenPassShared.h"

#define DRAWMODE_LIT 0
#define DRAWMODE_COLOR 1
#define DRAWMODE_NORMAL 2
#define DRAWMODE_ROUGHNESS 3
#define DRAWMODE_METALLIC 4
#define DRAWMODE_DEPTH 5
#define DRAWMODE_POSITION 6
#define DRAWMODE_LIGHTING 7
#define DRAWMODE_RTSHADOWS 8
#define DRAWMODE_VELOCITY 9
#define DRAWMODE_DISOCCLUSION 10
#define DRAWMODE_STAO 11

ConstantBuffer<DeferredData> c_Deferred : register(b1);
Texture2D<float4> t_tex2d_f4[8192] : register(t0, space0);
Texture2D<float2> t_tex2d_f2[8192] : register(t0, space1);
Texture2D<float> t_tex2d_f1[8192] : register(t0, space2);
SamplerState ClampedSampler : register(s1);

struct PS_OUTPUT
{
    float4 Color : SV_TARGET0;
};

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
    
    float3 Position = GetWorldPosFromScreen(c_Deferred.CamToWorld, (Input.SVPosition.xy * c_Deferred.ViewportSizeRcp) * 2.0f - 1.0f, Depth);

    float Shadow = t_tex2d_f1[c_Deferred.ShadowTextureIndex].SampleLevel(ClampedSampler, Input.UV, 0u).r;

    [branch]
    if(c_Deferred.DrawMode == DRAWMODE_COLOR)
    {
        Output.Color = float4(Color.rgb, 1.0f);
    }
    else if(c_Deferred.DrawMode == DRAWMODE_NORMAL)
    {
        Output.Color = float4((Normal + float3(1, 1, 1)) * 0.5f, 1);
    }
    else if(c_Deferred.DrawMode == DRAWMODE_ROUGHNESS)
    {
        Output.Color = float4(RoughnessMetallic.rrr, 1.0f);
    }
    else if(c_Deferred.DrawMode == DRAWMODE_METALLIC)
    {
        Output.Color = float4(RoughnessMetallic.ggg, 1.0f);
    }
    else if(c_Deferred.DrawMode == DRAWMODE_DEPTH)
    {
        Output.Color = float4(Depth.rrr, 1.0f);
    }
    else if(c_Deferred.DrawMode == DRAWMODE_POSITION)
    {
        Output.Color = float4(frac(abs(Position)), 1.0f);
    }
    else if(c_Deferred.DrawMode == DRAWMODE_RTSHADOWS)
    {
        Output.Color = float4(Shadow, Shadow, Shadow, 1.0f);
    }
    else if(c_Deferred.DrawMode == DRAWMODE_VELOCITY)
    {
        float2 Velocity = t_tex2d_f2[c_Deferred.SceneVelocityTextureIndex].SampleLevel(ClampedSampler, Input.UV, 0u).rg;
        Output.Color = float4((Velocity * 0.5 + 0.5),0, 1);
        //Output.Color = float4(abs(Velocity) * 10, 0, 1);
    }
    else if(c_Deferred.DrawMode == DRAWMODE_DISOCCLUSION)
    {
        float Confidence =t_tex2d_f1[c_Deferred.ConfidenceTextureIndex].SampleLevel(ClampedSampler, Input.UV, 0u).r;
        Output.Color = float4(Confidence.rrr, 1);
    }
    else if(c_Deferred.DrawMode == DRAWMODE_STAO)
    {
        float3 AO = t_tex2d_f4[c_Deferred.STAOTextureIndex].SampleLevel(ClampedSampler, Input.UV, 0u).rgb;
        Output.Color = float4(AO, 1);
    }
    else
    {
        Output.Color = float4(0, 0, 0, 1);
    }
}
#endif