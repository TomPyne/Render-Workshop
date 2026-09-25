#pragma once

struct ViewUniforms_s
{
    float4x4 ViewProjectionMatrix;

    float3 CamPos;
    float Time;

    float2 InvViewportSize;
    float2 Pad0;
};