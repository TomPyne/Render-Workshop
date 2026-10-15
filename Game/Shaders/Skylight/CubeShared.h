#pragma once

static const float PI = 3.14159265f;

// D3D cube face order (+X, -X, +Y, -Y, +Z, -Z). UV is in [0, 1] with v down, the result is not normalized
float3 CubeFaceDirection(uint Face, float2 UV)
{
    const float2 P = UV * 2.0f - 1.0f;

    switch (Face)
    {
    case 0:     return float3(1.0f, -P.y, -P.x);
    case 1:     return float3(-1.0f, -P.y, P.x);
    case 2:     return float3(P.x, 1.0f, P.y);
    case 3:     return float3(P.x, -1.0f, -P.y);
    case 4:     return float3(P.x, -P.y, 1.0f);
    default:    return float3(-P.x, -P.y, -1.0f);
    }
}
