#ifndef VIEW_H
#define VIEW_H

struct ViewData
{
    float4x4 ViewProjectionMatrix;
    float3 CameraPos;
    float __pad;
};

#endif