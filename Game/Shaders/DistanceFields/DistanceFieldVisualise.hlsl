#ifdef _CS

// Must match DistanceFieldVisualiseMode_e
#define VISUALISE_GLOBAL_SLICE 0
#define VISUALISE_GLOBAL_TRACE 1

// Must match DistanceFieldTraceView_e
#define TRACE_VIEW_SHADED 0
#define TRACE_VIEW_STEP_HEATMAP 1

struct Uniforms_s
{
    float4x4 InvViewProjection;

    uint2 OutputSize;
    uint OutputTexture;
    uint Mode;

    float3 VolumeMin;
    float VoxelSize;

    uint3 VolumeResolution;
    float Band;

    uint VolumeTexture;
    uint SliceIndex;
    uint MaxSteps;
    uint TraceView;

    uint ExitTint;
    // World size of the grid overlay cells, 0 when the grid is off
    float GridCellSize;
    float2 __Pad;
};

ConstantBuffer<Uniforms_s> c_Uniforms : register(b0);

Texture3D<float> t_tex3d_f1[8192] : register(t1, space0);
RWTexture2D<float4> u_tex2d_f4[8192] : register(u0, space0);

SamplerState s_LinearClamp : register(s1);

static const float3 kBackground = float3(0.1f, 0.1f, 0.1f);
static const float3 kExitTint = float3(0.12f, 0.16f, 0.28f);
static const float3 kOutOfSteps = float3(1.0f, 0.0f, 1.0f);

// Fractions of a voxel
static const float kHitThreshold = 0.25f;
static const float kMinStep = 0.1f;
static const uint kRefineSteps = 4;

// Fraction of a grid cell
static const float kGridLineWidth = 0.06f;
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

float SampleGlobal(float3 WorldPos)
{
    const float3 UVW = (WorldPos - c_Uniforms.VolumeMin) / (c_Uniforms.VoxelSize * float3(c_Uniforms.VolumeResolution));
    return t_tex3d_f1[c_Uniforms.VolumeTexture].SampleLevel(s_LinearClamp, UVW, 0.0f);
}

float3 GlobalNormal(float3 WorldPos, float3 Fallback)
{
    const float Offset = c_Uniforms.VoxelSize;
    const float3 Gradient = float3(
        SampleGlobal(WorldPos + float3(Offset, 0.0f, 0.0f)) - SampleGlobal(WorldPos - float3(Offset, 0.0f, 0.0f)),
        SampleGlobal(WorldPos + float3(0.0f, Offset, 0.0f)) - SampleGlobal(WorldPos - float3(0.0f, Offset, 0.0f)),
        SampleGlobal(WorldPos + float3(0.0f, 0.0f, Offset)) - SampleGlobal(WorldPos - float3(0.0f, 0.0f, Offset)));

    const float LengthSq = dot(Gradient, Gradient);
    return LengthSq > 1e-12f ? Gradient * rsqrt(LengthSq) : Fallback;
}

// Blue through green and yellow to red as Fraction goes from 0 to 1
float3 HeatmapColour(float Fraction)
{
    const float X = saturate(Fraction) * 3.0f;
    if (X < 1.0f)
        return lerp(float3(0.0f, 0.0f, 1.0f), float3(0.0f, 1.0f, 0.0f), X);
    if (X < 2.0f)
        return lerp(float3(0.0f, 1.0f, 0.0f), float3(1.0f, 1.0f, 0.0f), X - 1.0f);
    return lerp(float3(1.0f, 1.0f, 0.0f), float3(1.0f, 0.0f, 0.0f), X - 2.0f);
}

// 1 on grid lines, 0 elsewhere. Lines on the axis the normal faces are skipped, otherwise faces aligned
// with the grid would be covered.
float GridLine(float3 WorldPos, float3 Normal)
{
    const float3 Cell = (WorldPos - c_Uniforms.VolumeMin) / c_Uniforms.GridCellSize;
    const float3 EdgeDistance = abs(frac(Cell + 0.5f) - 0.5f);

    const float3 AbsNormal = abs(Normal);
    const float3 Facing = step(max(AbsNormal.yzx, AbsNormal.zxy), AbsNormal);

    const float3 OnLine = step(EdgeDistance, kGridLineWidth) * (1.0f - Facing);
    return max(max(OnLine.x, OnLine.y), OnLine.z);
}

// Sphere traces the camera ray through the global volume
float3 GlobalTrace(uint2 PixelCoord)
{
    const float2 UV = (float2(PixelCoord) + 0.5f) / float2(c_Uniforms.OutputSize);
    const float2 NDC = float2(UV.x * 2.0f - 1.0f, 1.0f - UV.y * 2.0f);

    const float4 NearH = mul(c_Uniforms.InvViewProjection, float4(NDC, 0.0f, 1.0f));
    const float4 FarH = mul(c_Uniforms.InvViewProjection, float4(NDC, 1.0f, 1.0f));
    const float3 RayOrigin = NearH.xyz / NearH.w;
    const float3 RayEnd = FarH.xyz / FarH.w;
    const float RayLength = length(RayEnd - RayOrigin);
    const float3 RayDir = (RayEnd - RayOrigin) / RayLength;

    const float3 VolumeMax = c_Uniforms.VolumeMin + c_Uniforms.VoxelSize * float3(c_Uniforms.VolumeResolution);
    const float3 InvDir = 1.0f / RayDir;
    const float3 T0 = (c_Uniforms.VolumeMin - RayOrigin) * InvDir;
    const float3 T1 = (VolumeMax - RayOrigin) * InvDir;
    const float3 TNear = min(T0, T1);
    const float3 TFar = max(T0, T1);
    const float TEnter = max(max(max(TNear.x, TNear.y), TNear.z), 0.0f);
    const float TExit = min(min(min(TFar.x, TFar.y), TFar.z), RayLength);

    if (TEnter >= TExit)
    {
        return kBackground;
    }

    const float HitThreshold = kHitThreshold * c_Uniforms.VoxelSize;
    const float MinStep = kMinStep * c_Uniforms.VoxelSize;

    float T = TEnter;
    uint Steps = 0;
    bool Hit = false;
    for (; Steps < c_Uniforms.MaxSteps; Steps++)
    {
        if (T > TExit)
            break;

        const float Distance = SampleGlobal(RayOrigin + RayDir * T);
        if (Distance < HitThreshold)
        {
            Hit = true;
            break;
        }

        T += max(Distance, MinStep);
    }

    if (c_Uniforms.TraceView == TRACE_VIEW_STEP_HEATMAP)
    {
        return HeatmapColour(float(Steps) / float(c_Uniforms.MaxSteps));
    }

    if (!Hit)
    {
        if (Steps == c_Uniforms.MaxSteps)
            return kOutOfSteps;

        return c_Uniforms.ExitTint != 0 ? kExitTint : kBackground;
    }

    // The hit test fires anywhere inside the threshold, depending on where the steps happened to land, which makes
    // the surface swim as the camera moves. A few unclamped steps, backwards when inside, pull it onto the surface.
    for (uint RefineIt = 0; RefineIt < kRefineSteps; RefineIt++)
    {
        T = max(T + SampleGlobal(RayOrigin + RayDir * T), TEnter);
    }

    const float3 HitPos = RayOrigin + RayDir * T;
    const float3 Normal = GlobalNormal(HitPos, -RayDir);

    // Headlight, so everything visible is lit
    float3 Colour = 0.75f * (0.15f + 0.85f * saturate(dot(Normal, -RayDir)));

    if (c_Uniforms.GridCellSize > 0.0f)
    {
        Colour *= 1.0f - 0.6f * GridLine(HitPos, Normal);
    }

    return Colour;
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
    else if (c_Uniforms.Mode == VISUALISE_GLOBAL_TRACE)
    {
        Colour = GlobalTrace(PixelCoord);
    }

    u_tex2d_f4[c_Uniforms.OutputTexture][PixelCoord] = float4(Colour, 1.0f);
}

#endif // #ifdef _CS
