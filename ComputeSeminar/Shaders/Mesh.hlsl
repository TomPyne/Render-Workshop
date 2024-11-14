#include "BallData.h"
#include "Samplers.h"
#include "Scene.h"
#include "View.h"

ConstantBuffer<SceneData> c_SceneData : register(b1);
StructuredBuffer<BallData> t_BallDataBuf[512] : register(t0, space0);
StructuredBuffer<uint> t_BallIndexBuf[512] : register(t0, space1);

struct PS_INPUT
{
    float4 SVPosition : SV_POSITION;
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

ConstantBuffer<ViewData> c_View : register(b0);

struct VS_INPUT
{
    float3 Position : POSITION;
};

void main(in VS_INPUT Input, in uint InstanceID : SV_InstanceID, out PS_INPUT Output)
{
    uint BallIndex = LoadBallIndex(InstanceID);
    Output.BallIndex = BallIndex;

    BallData Ball = LoadBallData(BallIndex);
    float3 WorldPos = (Input.Position * Ball.Scale) + Ball.Position;
    
    Output.SVPosition = mul(c_View.ViewProjectionMatrix, float4(WorldPos, 1.0f));    
};

#endif

#ifdef _PS

float4 main(in PS_INPUT Input) : SV_Target0
{
    BallData Ball = LoadBallData(Input.BallIndex);

    return float4(Ball.Color, 1.0f);
};

#endif