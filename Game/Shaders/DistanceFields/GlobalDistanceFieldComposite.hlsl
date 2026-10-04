#ifdef _CS

struct Uniforms_s
{
    float3 VolumeMin;
    float VoxelSize;

    uint3 Resolution;
    float Band;

    uint InstanceBufferIndex;
    uint InstanceCount;
    uint OutVolumeTexture;
    float __Pad;
};

// Must match DistanceFieldInstance_s in DistanceFieldScene.h
struct DistanceFieldInstance_s
{
    float4 WorldToVolume[3];

    float3 WorldBoundsMin;
    uint SDFTextureIndex;

    float3 WorldBoundsMax;
    float MaxDistance;

    float3 VolumeSize;
    float DistanceScale;
};

ConstantBuffer<Uniforms_s> c_Uniforms : register(b0);

StructuredBuffer<DistanceFieldInstance_s> t_sbuf_dfinstance[8192] : register(t1, space0);
Texture3D<float> t_tex3d_f1[8192] : register(t1, space1);
RWTexture3D<float> u_tex3d_f1[8192] : register(u0, space0);

SamplerState s_LinearClamp : register(s1);

// World space distance from WorldPos to the instance's surface. Outside the instance's volume this is
// the distance to the volume plus the distance stored at the nearest point on it.
float SampleInstance(DistanceFieldInstance_s Instance, float3 WorldPos)
{
    const float4 Point = float4(WorldPos, 1.0f);
    const float3 UVW = float3(dot(Instance.WorldToVolume[0], Point), dot(Instance.WorldToVolume[1], Point), dot(Instance.WorldToVolume[2], Point));
    const float3 ClampedUVW = saturate(UVW);

    const float OutsideDistance = length((UVW - ClampedUVW) * Instance.VolumeSize);

    const float Encoded = t_tex3d_f1[Instance.SDFTextureIndex].SampleLevel(s_LinearClamp, ClampedUVW, 0.0f);
    const float StoredDistance = (Encoded * 255.0f - 128.0f) / 127.0f * Instance.MaxDistance;

    return (OutsideDistance + StoredDistance) * Instance.DistanceScale;
}

[NumThreads(4, 4, 4)]
void main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    if (any(DispatchThreadID >= c_Uniforms.Resolution))
        return;

    const float3 VoxelCentre = c_Uniforms.VolumeMin + (float3(DispatchThreadID) + 0.5f) * c_Uniforms.VoxelSize;
    const float BandSq = c_Uniforms.Band * c_Uniforms.Band;

    float Distance = c_Uniforms.Band;

    for (uint InstanceIt = 0; InstanceIt < c_Uniforms.InstanceCount; InstanceIt++)
    {
        const DistanceFieldInstance_s Instance = t_sbuf_dfinstance[c_Uniforms.InstanceBufferIndex][InstanceIt];

        // Anything a band or more away from the instance's bounds would be clamped to the band anyway
        const float3 BoundsDelta = max(max(Instance.WorldBoundsMin - VoxelCentre, VoxelCentre - Instance.WorldBoundsMax), 0.0f);
        if (dot(BoundsDelta, BoundsDelta) >= BandSq)
            continue;

        Distance = min(Distance, SampleInstance(Instance, VoxelCentre));
    }

    u_tex3d_f1[c_Uniforms.OutVolumeTexture][DispatchThreadID] = clamp(Distance, -c_Uniforms.Band, c_Uniforms.Band);
}

#endif // #ifdef _CS
