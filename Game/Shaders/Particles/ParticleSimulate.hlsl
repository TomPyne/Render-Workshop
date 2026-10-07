#include "ParticleCommon.h"

RWStructuredBuffer<Particle_s> u_sbuf_particle[8192] : register(u0, space0);
RWStructuredBuffer<uint> u_sbuf_u1[8192] : register(u0, space1);
RWStructuredBuffer<ParticleDrawArgs_s> u_sbuf_drawargs[8192] : register(u0, space2);

#ifdef PARTICLE_RESET_ARGS

[numthreads(1, 1, 1)]
void main()
{
    ParticleDrawArgs_s Args;
    Args.VertexCountPerInstance = 6u;
    Args.InstanceCount = 0u;
    Args.StartVertexLocation = 0u;
    Args.StartInstanceLocation = 0u;

    u_sbuf_drawargs[c_Particles.DrawArgsUAV][0] = Args;
}

#endif // PARTICLE_RESET_ARGS

#ifdef PARTICLE_SIMULATE

static const float PI = 3.14159265f;
static const float3 Gravity = float3(0.0f, -9.81f, 0.0f);

float3 SampleConeDirection(float3 Axis, float MaxAngle, inout uint Rng)
{
    const float Theta = NextRandom(Rng) * MaxAngle;
    const float Phi = NextRandom(Rng) * 2.0f * PI;

    const float3 Up = abs(Axis.y) < 0.999f ? float3(0.0f, 1.0f, 0.0f) : float3(1.0f, 0.0f, 0.0f);
    const float3 Tangent = normalize(cross(Up, Axis));
    const float3 Bitangent = cross(Axis, Tangent);

    float SinTheta, CosTheta;
    sincos(Theta, SinTheta, CosTheta);

    return Tangent * (SinTheta * cos(Phi)) + Bitangent * (SinTheta * sin(Phi)) + Axis * CosTheta;
}

[numthreads(64, 1, 1)]
void main(uint3 DispatchThreadId : SV_DispatchThreadID)
{
    const uint Index = DispatchThreadId.x;

    if (Index >= c_Particles.MaxCount)
        return;

    Particle_s Particle = u_sbuf_particle[c_Particles.ParticlesUAV][Index];

    const uint SpawnOffset = (Index + c_Particles.MaxCount - c_Particles.SpawnStart) % c_Particles.MaxCount;

    if (SpawnOffset < c_Particles.SpawnCount)
    {
        uint Rng = PCGHash(Index ^ PCGHash(c_Particles.Seed ^ PCGHash(c_Particles.FrameIndex)));

        const float3 Direction = SampleConeDirection(normalize(c_Particles.SpawnDirection), c_Particles.MaxAngleRadians, Rng);
        const float Speed = lerp(c_Particles.VelocityMin, c_Particles.VelocityMax, NextRandom(Rng));

        Particle.Position = c_Particles.EmitterPosition;
        Particle.Age = 0.0f;
        Particle.Velocity = Direction * Speed;
        Particle.Lifetime = c_Particles.Lifetime;
    }
    else if (Particle.Age < Particle.Lifetime)
    {
        Particle.Velocity += Gravity * c_Particles.DeltaTime;
        Particle.Position += Particle.Velocity * c_Particles.DeltaTime;
        Particle.Age += c_Particles.DeltaTime;
    }
    else
    {
        return;
    }

    u_sbuf_particle[c_Particles.ParticlesUAV][Index] = Particle;

    if (Particle.Age < Particle.Lifetime)
    {
        uint Slot;
        InterlockedAdd(u_sbuf_drawargs[c_Particles.DrawArgsUAV][0].InstanceCount, 1u, Slot);
        u_sbuf_u1[c_Particles.AliveListUAV][Slot] = Index;
    }
}

#endif // PARTICLE_SIMULATE
