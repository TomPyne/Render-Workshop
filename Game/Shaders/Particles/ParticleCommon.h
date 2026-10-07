#pragma once

// Must match Particle_s in ParticleRenderer.cpp
struct Particle_s
{
    float3 Position;
    float Age;

    float3 Velocity;
    float Lifetime; // Age >= Lifetime is dead
};

// Must match ParticleDrawArgs_s in ParticleRenderer.cpp, laid out as indirect draw arguments
struct ParticleDrawArgs_s
{
    uint VertexCountPerInstance;
    uint InstanceCount;
    uint StartVertexLocation;
    uint StartInstanceLocation;
};

// Must match ParticleSystemUniforms_s in ParticleRenderer.cpp
struct ParticleSystemUniforms_s
{
    float3 EmitterPosition;
    float DeltaTime;

    float3 SpawnDirection;
    float MaxAngleRadians;

    float VelocityMin;
    float VelocityMax;
    float Lifetime;
    float Scale;

    uint MaxCount;
    uint SpawnStart; // First ring slot spawned this frame
    uint SpawnCount;
    uint Seed;

    uint FrameIndex;
    uint ParticlesUAV;
    uint AliveListUAV;
    uint DrawArgsUAV;

    uint ParticlesSRV;
    uint AliveListSRV;
    uint2 __Pad;
};

ConstantBuffer<ParticleSystemUniforms_s> c_Particles : register(b2);

uint PCGHash(uint Input)
{
    const uint State = Input * 747796405u + 2891336453u;
    const uint Word = ((State >> ((State >> 28u) + 4u)) ^ State) * 277803737u;
    return (Word >> 22u) ^ Word;
}

// Returns [0, 1) and advances State
float NextRandom(inout uint State)
{
    State = PCGHash(State);
    return float(State >> 8u) * (1.0f / 16777216.0f);
}
