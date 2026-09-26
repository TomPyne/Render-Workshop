#pragma once

struct DynamicUniforms_s
{
    float4x4 ModelMatrix;
    float4x4 NormalMatrix;
    float DeterminantSign;
    float3 __Pad;
};

struct ModelUniforms_s
{
    uint PositionBufferIndex;
    uint NormalBufferIndex;
    uint TangentBufferIndex;
    uint Texcoord0BufferIndex;
    uint Texcoord1BufferIndex;
    uint Texcoord2BufferIndex;
    uint Texcoord3BufferIndex;
    uint __Pad;
};