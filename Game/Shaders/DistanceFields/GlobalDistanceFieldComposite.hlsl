#ifdef _CS

struct Uniforms_s
{
    float3 VolumeMin;
    float VoxelSize;

    uint3 Resolution;
    float Band;

    float3 TestSphereCentre;
    float TestSphereRadius;

    uint OutVolumeTexture;
    float3 __Pad;
};

ConstantBuffer<Uniforms_s> c_Uniforms : register(b0);

RWTexture3D<float> u_tex3d_f1[8192] : register(u0, space0);

// Placeholder fill until the instance composite replaces it: a sphere at the volume centre
[NumThreads(4, 4, 4)]
void main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    if (any(DispatchThreadID >= c_Uniforms.Resolution))
        return;

    const float3 VoxelCentre = c_Uniforms.VolumeMin + (float3(DispatchThreadID) + 0.5f) * c_Uniforms.VoxelSize;
    const float Distance = length(VoxelCentre - c_Uniforms.TestSphereCentre) - c_Uniforms.TestSphereRadius;

    u_tex3d_f1[c_Uniforms.OutVolumeTexture][DispatchThreadID] = clamp(Distance, -c_Uniforms.Band, c_Uniforms.Band);
}

#endif // #ifdef _CS
