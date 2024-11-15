#include "BallData.h"
#include "Samplers.h"
#include "Scene.h"

ConstantBuffer<SceneData> c_SceneData : register(b0, space0);
RWStructuredBuffer<BallData> u_BallData[512] : register(u0, space0);
RWStructuredBuffer<uint> u_BallIndices[512] : register(u0, space1);
Texture2D<float> t_tex2d_float[512] : register(t0, space0);

[NumThreads(128, 1, 1)]
void main(uint3 DispatchThreadId : SV_DispatchThreadID)
{
    if(DispatchThreadId.x >= c_SceneData.NumBalls)
    {
        return;
    }

    u_BallIndices[c_SceneData.BallIndexUAVIndex][DispatchThreadId.x]  = DispatchThreadId.x;

    BallData Ball = u_BallData[c_SceneData.BallDataUAVIndex][DispatchThreadId.x];

    const float DeltaSeconds = c_SceneData.DeltaSeconds;
    const float Gravity = c_SceneData.Gravity * DeltaSeconds;

    const float VelocityMagSqr = dot(Ball.Velocity, Ball.Velocity);

    Ball.Velocity += float3(0.0f, Gravity, 0.0f);
    Ball.Velocity += sign(Ball.Velocity) * -c_SceneData.DragCoefficient;

    Ball.Position += Ball.Velocity * DeltaSeconds;

    const float TerrainScale = c_SceneData.TerrainScale;

    const float2 MinTerrainUV = saturate((Ball.Position.xz - Ball.Scale.xx) / TerrainScale);
    const float2 MaxTerrainUV = saturate((Ball.Position.xz + Ball.Scale.xx) / TerrainScale);

    const uint NoiseDim = c_SceneData.NoiseDim;

    const uint2 MinTerrainCoord = min(MinTerrainUV * NoiseDim, (NoiseDim - 1).xx);
    const uint2 MaxTerrainCoord = min(MaxTerrainUV * NoiseDim, (NoiseDim - 1).xx);

    const float NoiseDimRcp = rcp(NoiseDim);
    const float BallRadiusSqr = Ball.Scale * Ball.Scale;

    float3 BounceDir = 0;

    [loop]
    for(uint y = MinTerrainCoord.y; y <= MaxTerrainCoord.y; y++)
    {
        [loop]
        for(uint x = MinTerrainCoord.x; x <= MaxTerrainCoord.x; x++)
        {
            float Height = t_tex2d_float[c_SceneData.NoiseTexSRVIndex].Load(uint3(x, y, 0)).r * c_SceneData.TerrainHeight;
            float3 SamplePos = float3((x * NoiseDimRcp) * TerrainScale, Height, (y * NoiseDimRcp) * TerrainScale);

            float3 HitDirection = Ball.Position - SamplePos;
            float PenetrationSqr = BallRadiusSqr - dot(HitDirection, HitDirection);

            BounceDir += HitDirection * max(PenetrationSqr, 0.0f);
        }
    }
    if(length(BounceDir) > 0)
    {
        BounceDir = normalize(BounceDir);

        Ball.Velocity = BounceDir * length(Ball.Velocity) * Ball.Bounciness;
    }

    u_BallData[c_SceneData.BallDataUAVIndex][DispatchThreadId.x] = Ball;
}