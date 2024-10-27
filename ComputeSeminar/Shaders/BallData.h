#ifndef BALL_DATA_H
#define BALL_DATA_H

struct BallData
{
    float3 Position;
    float Scale;
    float3 Color;
    float Bounciness;
    float3 Velocity;
    float __pad;
};

#endif