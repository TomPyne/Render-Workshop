#pragma once

#ifndef PI
#define PI 3.141593f;
#endif

/*
* Library of different random functions and generators
*/

float Random(float3 Seed)
{
    return frac(sin(dot(Seed.xyz, float3(12.9898,78.233, 128.943)))* 43758.5453123);
}

float Random(float2 Seed)
{
    return frac(sin(dot(Seed.xy, float2(12.9898,78.233)))* 43758.5453123);
}

float2 Random2(float2 Seed)
{
    return float2(Random(Seed), Random(Seed + 1.234));
}

float3 GetRandomHemisphere_Uniform(float2 Seed)
{
    const float2 Random = Random2(Seed);
    const float Phi = Random.x * 2.0f * PI;
    const float CosTheta = Random.y;
    const float SinTheta = sqrt(1.0f - CosTheta * CosTheta);
    return float3(cos(Phi) * SinTheta, sin(Phi) * SinTheta, CosTheta);
}

float3 GetRandomHemisphere_Cosine(float2 Seed)
{
    const float2 Random = Random2(Seed);
    float phi = 2.0 * 3.14159265 * Random.x;
    float cosTheta = sqrt(1.0 - Random.y);
    float sinTheta = sqrt(Random.y);

    float x = cos(phi) * sinTheta;
    float y = sin(phi) * sinTheta;
    float z = cosTheta;

    return float3(x, y, z);
}