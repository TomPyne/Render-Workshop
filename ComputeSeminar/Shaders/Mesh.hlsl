#include "BallData.h"
#include "Samplers.h"
#include "Scene.h"
#include "View.h"

ConstantBuffer<SceneData> c_SceneData : register(b1);
StructuredBuffer<BallData> t_BallData[512] : register(t0, space0);

struct PS_INPUT
{
    float4 SVPosition : SV_POSITION;
    uint InstanceID : INSTANCEID;
};

BallData LoadInstanceBallData(uint InstanceID)
{
    return t_BallData[c_SceneData.BallDataSRVIndex][InstanceID];
}

#ifdef _VS

ConstantBuffer<ViewData> c_View : register(b0);

struct VS_INPUT
{
    float3 Position : POSITION;
};

void main(in VS_INPUT Input, in uint InstanceID : SV_InstanceID, out PS_INPUT Output)
{
    BallData Ball = LoadInstanceBallData(InstanceID);
    float3 WorldPos = (Input.Position * Ball.Scale) + Ball.Position;
    
    Output.SVPosition = mul(c_View.ViewProjectionMatrix, float4(WorldPos, 1.0f));
    Output.InstanceID = InstanceID;
};

#endif

#ifdef _PS

float4 main(in PS_INPUT Input) : SV_Target0
{
    BallData Ball = LoadInstanceBallData(Input.InstanceID);

    return float4(Ball.Color, 1.0f);
};

#endif