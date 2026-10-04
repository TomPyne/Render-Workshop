#ifdef _CS

// Must match DistanceFieldVisualiseMode_e
#define VISUALISE_GLOBAL_SLICE 0

struct Uniforms_s
{
    uint2 OutputSize;
    uint OutputTexture;
    uint Mode;

    float3 VolumeMin;
    float VoxelSize;

    uint3 VolumeResolution;
    float Band;

    uint VolumeTexture;
    uint SliceIndex;
    float2 __Pad;
};

ConstantBuffer<Uniforms_s> c_Uniforms : register(b0);

Texture3D<float> t_tex3d_f1[8192] : register(t1, space0);
RWTexture2D<float4> u_tex2d_f4[8192] : register(u0, space0);

static const float3 kBackground = float3(0.1f, 0.1f, 0.1f);
static const float kContourSpacingVoxels = 4.0f;

// Blue inside, orange outside, white within half a voxel of the surface, with contour bands
float3 DistanceColour(float Distance)
{
    if (abs(Distance) < c_Uniforms.VoxelSize * 0.5f)
    {
        return 1.0f;
    }

    const float3 Base = Distance < 0.0f ? float3(0.25f, 0.55f, 1.0f) : float3(1.0f, 0.6f, 0.2f);
    const float Falloff = 1.0f - 0.6f * saturate(abs(Distance) / c_Uniforms.Band);
    const float Contour = 0.8f + 0.2f * cos(2.0f * 3.14159265f * Distance / (c_Uniforms.VoxelSize * kContourSpacingVoxels));

    return Base * Falloff * Contour;
}

// One horizontal slice of the volume, X to the right and Z up, letterboxed to the output
float3 GlobalSlice(uint2 PixelCoord)
{
    const uint2 SliceSize = c_Uniforms.VolumeResolution.xz;
    const float Scale = min(float(c_Uniforms.OutputSize.x) / SliceSize.x, float(c_Uniforms.OutputSize.y) / SliceSize.y);
    const float2 Offset = (float2(c_Uniforms.OutputSize) - float2(SliceSize) * Scale) * 0.5f;

    const float2 SlicePos = (float2(PixelCoord) + 0.5f - Offset) / Scale;
    if (any(SlicePos < 0.0f) || any(SlicePos >= float2(SliceSize)))
    {
        return kBackground;
    }

    const uint3 Voxel = uint3(uint(SlicePos.x), c_Uniforms.SliceIndex, SliceSize.y - 1 - uint(SlicePos.y));
    return DistanceColour(t_tex3d_f1[c_Uniforms.VolumeTexture].Load(int4(Voxel, 0)));
}

[NumThreads(8, 8, 1)]
void main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    const uint2 PixelCoord = DispatchThreadID.xy;
    if (any(PixelCoord >= c_Uniforms.OutputSize))
        return;

    float3 Colour = kBackground;
    if (c_Uniforms.Mode == VISUALISE_GLOBAL_SLICE)
    {
        Colour = GlobalSlice(PixelCoord);
    }

    u_tex2d_f4[c_Uniforms.OutputTexture][PixelCoord] = float4(Colour, 1.0f);
}

#endif // #ifdef _CS
