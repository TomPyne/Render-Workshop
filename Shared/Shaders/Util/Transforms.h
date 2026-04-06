#pragma once

/*
* Library of general transforms
*/

float3 GetViewPosFromScreen(float2 Pixel, float Depth, float4x4 InverseProjection, float2 ViewportSizeRcp)
{    
    float2 NDC = (Pixel * ViewportSizeRcp) * 2.0f - 1.0f;
    NDC.y = -NDC.y;
    float4 Projected = float4(NDC, Depth, 1.0f);
    float4 Unprojected = mul( Projected, InverseProjection);
    return Unprojected.xyz / Unprojected.w;
}

float3x3 ComputeBasisMatrix(float3 Normal)
{
    Normal = normalize(Normal);
    const float3 Up = abs(Normal.z) < 0.999 ? float3(0,0,1) : float3(1,0,0);
    const float3 Tangent = normalize(cross(Up, Normal));
    const float3 Bitangent = normalize(cross(Normal, Tangent));

    return transpose(float3x3(Tangent, Bitangent, Normal));
}