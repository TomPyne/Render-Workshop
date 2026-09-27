#pragma once

#include "ShaderDefines.h"

#include "Model.h"
#include "View.h"

ConstantBuffer<DynamicUniforms_s> c_Dynamic : register(b0);
ConstantBuffer<ViewUniforms_s> c_View : register(b1);
ConstantBuffer<ModelUniforms_s> c_Model : register(b2);
ConstantBuffer<MaterialUniforms_s> c_Material : register(b3);

#ifdef _VS
StructuredBuffer<float2> t_sbuf_f2[8192] : register(t1, space0);
StructuredBuffer<float3> t_sbuf_f3[8192] : register(t1, space1);
StructuredBuffer<float4> t_sbuf_f4[8192] : register(t1, space2);

float3 LoadPosition(uint VertexID)
{
    return t_sbuf_f3[c_Model.PositionBufferIndex][VertexID];
}

float3 LoadNormal(uint VertexID)
{
    return t_sbuf_f3[c_Model.NormalBufferIndex][VertexID];
}

float4 LoadTangent(uint VertexID)
{
    return t_sbuf_f4[c_Model.TangentBufferIndex][VertexID];
}

float2 LoadUV0(uint VertexID)
{
    return t_sbuf_f2[c_Model.Texcoord0BufferIndex][VertexID];
}

float2 LoadUV1(uint VertexID)
{
    if (c_Model.Texcoord1BufferIndex == 0)
    {
        return float2(0.0f, 0.0f);
    }

    return t_sbuf_f2[c_Model.Texcoord1BufferIndex][VertexID];
}

float2 LoadUV2(uint VertexID)
{
    if (c_Model.Texcoord2BufferIndex == 0)
    {
        return float2(0.0f, 0.0f);
    }

    return t_sbuf_f2[c_Model.Texcoord2BufferIndex][VertexID];
}

float2 LoadUV3(uint VertexID)
{
    if (c_Model.Texcoord3BufferIndex == 0)
    {
        return float2(0.0f, 0.0f);
    }

    return t_sbuf_f2[c_Model.Texcoord3BufferIndex][VertexID];
}

float3 ModelToWorld(float3 ModelPosition)
{
    return mul(c_Dynamic.ModelMatrix, float4(ModelPosition, 1.0f)).xyz;
}

float4 WorldToClip(float3 WorldPosition)
{
    return mul(c_View.ViewProjectionMatrix, float4(WorldPosition, 1.0f));
}

float4 ModelToClip(float3 ModelPosition)
{
    return WorldToClip(ModelToWorld(ModelPosition));
}

float4 ModelToPrevClip(float3 ModelPosition)
{
    const float3 PrevWorldPosition = mul(c_Dynamic.PrevModelMatrix, float4(ModelPosition, 1.0f)).xyz;
    return mul(c_View.PrevViewProjectionMatrix, float4(PrevWorldPosition, 1.0f));
}

float3 NormalModelToWorld(float3 Normal)
{
    return normalize(mul((float3x3)c_Dynamic.NormalMatrix, Normal));
}

float4 TangentModelToWorld(float4 Tangent)
{
    return float4(
        normalize(mul((float3x3)c_Dynamic.ModelMatrix, Tangent.xyz)),
        Tangent.w * c_Dynamic.DeterminantSign
    );
}

void VS_PosNormal(in uint VertexID, out float4 Position, out float4 PrevPosition, out float3 Normal)
{
    const float3 ModelPosition = LoadPosition(VertexID);
    Position = ModelToClip(ModelPosition);
    PrevPosition = ModelToPrevClip(ModelPosition);
    Normal = NormalModelToWorld(LoadNormal(VertexID));
}

void VS_PosNormalUV0(in uint VertexID, out float4 Position, out float4 PrevPosition, out float3 Normal, out float2 UV0)
{
    const float3 ModelPosition = LoadPosition(VertexID);
    Position = ModelToClip(ModelPosition);
    PrevPosition = ModelToPrevClip(ModelPosition);
    Normal = NormalModelToWorld(LoadNormal(VertexID));
    UV0 = LoadUV0(VertexID);
}

void VS_PosNormalTangent(in uint VertexID, out float4 Position, out float4 PrevPosition, out float3 Normal, out float4 Tangent)
{
    const float3 ModelPosition = LoadPosition(VertexID);
    Position = ModelToClip(ModelPosition);
    PrevPosition = ModelToPrevClip(ModelPosition);
    Normal = NormalModelToWorld(LoadNormal(VertexID));
    Tangent = TangentModelToWorld(LoadTangent(VertexID));
}

void VS_PosNormalTangentUV0(in uint VertexID, out float4 Position, out float4 PrevPosition, out float3 Normal, out float4 Tangent, out float2 UV0)
{
    const float3 ModelPosition = LoadPosition(VertexID);
    Position = ModelToClip(ModelPosition);
    PrevPosition = ModelToPrevClip(ModelPosition);
    Normal = NormalModelToWorld(LoadNormal(VertexID));
    Tangent = TangentModelToWorld(LoadTangent(VertexID));
    UV0 = LoadUV0(VertexID);
}

void VS_PosNormalTangentUV0UV1(in uint VertexID, out float4 Position, out float4 PrevPosition, out float3 Normal, out float4 Tangent, out float2 UV0, out float2 UV1)
{
    const float3 ModelPosition = LoadPosition(VertexID);
    Position = ModelToClip(ModelPosition);
    PrevPosition = ModelToPrevClip(ModelPosition);
    Normal = NormalModelToWorld(LoadNormal(VertexID));
    Tangent = TangentModelToWorld(LoadTangent(VertexID));
    UV0 = LoadUV0(VertexID);
    UV1 = LoadUV1(VertexID);
}

void VS_PosNormalTangentUV0UV1(in uint VertexID, out float4 Position, out float4 PrevPosition, out float3 Normal, out float4 Tangent, out float2 UV0, out float2 UV1, out float2 UV2)
{
    const float3 ModelPosition = LoadPosition(VertexID);
    Position = ModelToClip(ModelPosition);
    PrevPosition = ModelToPrevClip(ModelPosition);
    Normal = NormalModelToWorld(LoadNormal(VertexID));
    Tangent = TangentModelToWorld(LoadTangent(VertexID));
    UV0 = LoadUV0(VertexID);
    UV1 = LoadUV1(VertexID);
    UV2 = LoadUV2(VertexID);
}

#endif // #ifdef _VS

#ifdef _PS

struct PSOutput_s
{
    float4 AlbedoMetallic : SV_TARGET0;
    float4 NormalRoughness : SV_TARGET1;
    float4 EmissiveSpecular : SV_Target2;
    float4 Velocity : SV_Target3;
};

float3 TangentToWorldNormals(float3 Normal, float3 VertexNormal, float4 Tangent)
{
    const float3 N = normalize(VertexNormal);
    const float3 T = normalize(Tangent.xyz - N * dot(N, Tangent.xyz));
    const float3 B = cross(N, T) * Tangent.w;

    return normalize(Normal.x * T + Normal.y * B + Normal.z * N);
}

// Velocity.xy is CurrentUV - PrevUV, so consumers find last frame's sample at UV - Velocity.xy
// Velocity.z is the surface's view depth last frame, to validate history against, 0 when invalid
void MaterialOutput_Velocity(float4 SvPosition, float4 PrevPosition, inout PSOutput_s Output)
{
    const float2 CurrentUV = SvPosition.xy * c_View.InvViewportSize;

    // Behind the camera last frame, push the previous sample off-screen so temporal effects reject it
    float2 PrevUV = float2(-1.0f, -1.0f);
    float PrevDepth = 0.0f;
    if (PrevPosition.w > 0.0f)
    {
        PrevUV = (PrevPosition.xy / PrevPosition.w) * float2(0.5f, -0.5f) + 0.5f;
        PrevDepth = PrevPosition.w;
    }

    Output.Velocity = float4(CurrentUV - PrevUV, PrevDepth, 0.0f);
}

// Initialises every output, call first as the other helpers only write their own fields
void MaterialOutput_Default(float3 Normal, out PSOutput_s Output)
{
    Output.AlbedoMetallic = float4(0.0f, 0.0f, 0.0f, 0.0f);
    Output.NormalRoughness = float4(Normal, 0.5f);
    Output.EmissiveSpecular = float4(0.0f, 0.0f, 0.0f, 0.5f);
    Output.Velocity = float4(0.0f, 0.0f, 0.0f, 0.0f);
}

void MaterialOutput_Albedo(float3 Albedo, inout PSOutput_s Output)
{
    Output.AlbedoMetallic.rgb = Albedo;
}

void MaterialOutput_Metallic(float Metallic, inout PSOutput_s Output)
{
    Output.AlbedoMetallic.a = Metallic;
}

void MaterialOutput_Roughness(float Roughness, inout PSOutput_s Output)
{
    Output.NormalRoughness.a = Roughness;
}

void MaterialOutput_Specular(float Specular, inout PSOutput_s Output)
{
    Output.EmissiveSpecular.a = Specular;
}

void MaterialOutput_Emissive(float3 Emissive, inout PSOutput_s Output)
{
    Output.EmissiveSpecular.rgb = Emissive;
}

void MaterialOutput_AmbientOcclusion(float AO, inout PSOutput_s Output)
{
    // TODO
}
#endif // #ifdef _PS