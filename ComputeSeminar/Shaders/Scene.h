#ifndef SCENE_H
#define SCENE_H

struct SceneData
{
    uint NoiseTexSRVIndex;
    uint BallDataUAVIndex;
    uint BallDataSRVIndex;
    uint FrameID;
    float TerrainScale;
    float TerrainHeight;
    float DeltaSeconds;
    float Gravity;
    uint NumBalls;
    float DragCoefficient;
    uint NoiseDim;
    float __pad;
};

#endif