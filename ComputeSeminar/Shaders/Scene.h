#ifndef SCENE_H
#define SCENE_H

#define PLANE_TOP 0u
#define PLANE_RIGHT 1u
#define PLANE_BOTTOM 2u
#define PLANE_LEFT 3u
#define PLANE_NEAR 4u
#define PLANE_FAR 5u
#define PLANE_COUNT 6u

struct SceneData
{
    uint FrameID;
    float TerrainScale;
    float TerrainHeight;
    float DeltaSeconds;

    float Gravity;
    uint NumBalls;
    float DragCoefficient;
    uint NoiseDim;

    uint NoiseTexSRVIndex;
    uint BallDataUAVIndex;
    uint BallDataSRVIndex;
    uint BallIndexUAVIndex;

    uint BallIndexSRVIndex;
    uint IndirectDrawUAVIndex;
    float2 __pad;

    float4 FrustumPlanes[PLANE_COUNT];
};

#endif