#pragma once

struct DeferredData
{
    float4x4 CamToWorld;
    float4x4 PrevCamToWorld;

    uint SceneColorTextureIndex;
    uint SceneNormalTextureIndex;
    uint SceneRoughnessMetallicTextureIndex;
    uint DepthTextureIndex;

    uint DrawMode;
    float3 CamPosition;

    uint ShadowTextureIndex;
    float3 SunDirection;

    uint SceneVelocityTextureIndex;
    uint ConfidenceTextureIndex;
    float2 ViewportSizeRcp;
};

float3 GetWorldPosFromScreen(float4x4 CamToWorld, float2 ScreenPos, float Depth)
{
    float4 ProjectedPos = float4(ScreenPos, Depth,  1.0f);
    float4 Unprojected = mul(CamToWorld, ProjectedPos);
    return Unprojected.xyz / Unprojected.w;
}
