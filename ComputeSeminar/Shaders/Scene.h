#ifndef SCENE_H
#define SCENE_H

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
    float3 __pad;
};

#endif