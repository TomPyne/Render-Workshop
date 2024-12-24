#include "BallData.h"
#include "Samplers.h"
#include "Scene.h"
#include "View.h"

ConstantBuffer<ViewData> c_View : register(b0);
ConstantBuffer<SceneData> c_SceneData : register(b1);
StructuredBuffer<BallData> t_BallDataBuf[512] : register(t0, space0);
StructuredBuffer<uint> t_BallIndexBuf[512] : register(t0, space1);

struct PS_INPUT
{
    float4 SVPosition : SV_POSITION;
    float3 WorldPosition : WORLDPOS;
    float3 Normal : NORMAL;
    uint BallIndex : BALLINDEX;
};

BallData LoadBallData(uint Index)
{
    return t_BallDataBuf[c_SceneData.BallDataSRVIndex][Index];
}

uint LoadBallIndex(uint InstanceID)
{
    return t_BallIndexBuf[c_SceneData.BallIndexSRVIndex][InstanceID];
}

#ifdef _VS

struct VS_INPUT
{
    float3 Position : POSITION;
};

void main(in VS_INPUT Input, in uint InstanceID : SV_InstanceID, out PS_INPUT Output)
{
    uint BallIndex = LoadBallIndex(InstanceID);
    Output.BallIndex = BallIndex;

    BallData Ball = LoadBallData(BallIndex);
    Output.WorldPosition = (Input.Position * Ball.Scale) + Ball.Position;
    Output.Normal = normalize(Input.Position);
    Output.SVPosition = mul(c_View.ViewProjectionMatrix, float4(Output.WorldPosition, 1.0f));    
};

#endif

#ifdef _PS

float4 main(in PS_INPUT Input) : SV_Target0
{
    BallData Ball = LoadBallData(Input.BallIndex);

    float3 normal = normalize(Input.Normal);
    float3 lightDir = normalize(float3(0.5f, 1.0f, 0.5f));
    float3 lightColor = float3(1.0f, 1.0f, 1.0f) * 0.8f;
    float ambientStrength = 0.5f;
    float3 viewDir = normalize(c_View.CameraPos - Input.WorldPosition);
    float3 halfDir = normalize(lightDir + viewDir);

    float3 ambient  = ambientStrength * lightColor;
    float3 diff = max(dot(normal, lightDir), 0.0f) * lightColor;
    float3 spec = pow(max(dot(normal, halfDir), 0.0f), 16.0f) * lightColor;


    float3 finalColor = (ambient + diff + spec) * Ball.Color;
    finalColor *= 8;
    finalColor = floor(finalColor);
    finalColor *= 0.125f;



    return float4(finalColor, 1.0f);
};

#endif