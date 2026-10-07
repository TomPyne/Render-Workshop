#pragma once

struct ViewUniforms_s
{
    float4x4 ViewProjectionMatrix;
    float4x4 PrevViewProjectionMatrix;

    float3 CamPos;
    float Time;

    float2 InvViewportSize;
    float2 Pad0;

    float3 CamRight;
    float Pad1;

    float3 CamUp;
    float Pad2;
};