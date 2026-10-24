#pragma once

#ifndef USE_NORMAL
#define USE_NORMAL 0
#endif

#ifndef USE_UV0
#define USE_UV0 0
#endif

#ifndef USE_UV1
#define USE_UV1 0
#endif

#ifndef USE_UV2
#define USE_UV2 0
#endif

#ifndef USE_CUSTOM_UV0
#define USE_CUSTOM_UV0 0
#endif

#ifndef USE_CUSTOM_UV1
#define USE_CUSTOM_UV1 0
#endif

#ifndef USE_CUSTOM_UV2
#define USE_CUSTOM_UV2 0
#endif

#include "../../../Game/Shaders/MeshMaterial.h"

struct Interpolants_s
{
    float4 Position : SV_POSITION;
    float4 PrevPosition : PREV_POSITION;
#if USE_NORMAL
    float3 Normal : NORMAL;
    float4 Tangent : TANGENT;
#endif
#if USE_UV0
    float2 UV0 : TEXCOORD0;
#endif
#if USE_UV1
    float2 UV1 : TEXCOORD1;
#endif
#if USE_UV2
    float2 UV2 : TEXCOORD2;
#endif
};

#ifdef _VS

#if USE_UV0 && USE_CUSTOM_UV0
void SetUV0(float2 UV, inout Interpolants_s Output)
{
    Output.UV0 = UV;
}
#endif // #if USE_UV0

#if USE_UV1 && USE_CUSTOM_UV1
void SetUV1(float2 UV, inout Interpolants_s Output)
{
    Output.UV0 = UV;
}
#endif // #if USE_UV1

#if USE_UV2 && USE_CUSTOM_UV2
void SetUV2(float2 UV, inout Interpolants_s Output)
{
    Output.UV0 = UV;
}
#endif // #if USE_UV2

#endif // #ifdef _VS

#ifdef _PS

struct PixelShaderMaterialOutputs_s
{
    float3 Albedo;
    float Metallic;
    float Specular;
    float Roughness;
    float3 Emissive;
    float3 TangentNormal;
    float AmbientOcclusion;
};

#endif // #ifdef _PS