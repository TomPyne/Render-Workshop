#ifdef _CS

struct Uniforms_s
{
    float4x4 InvViewProjection;

    uint2 OutputSize;
    uint SceneDepthTexture;
    uint SceneNormalTexture;

    float3 VolumeMin;
    uint VolumeTexture;

    float3 VolumeMax;
    float Band;

    uint OutputTexture;
    uint Enabled;
    uint StepsPerCone;
    // Distance of the first sample along each cone, later samples are spaced geometrically by StepScale
    float FirstStep;

    float StepScale;
    // World distance the cone origins are pushed out along the normal
    float NormalBias;
    float Power;
    float __Pad;
};

ConstantBuffer<Uniforms_s> c_Uniforms : register(b0);

Texture2D<float4> t_tex2d_f4[8192] : register(t1, space0);
Texture3D<float> t_tex3d_f1[8192] : register(t1, space1);
RWTexture2D<float> u_tex2d_f1[8192] : register(u0, space0);

SamplerState s_LinearClamp : register(s1);

static const float PI = 3.14159265f;

// One cone along the normal and a ring of 8 around it. Each cone has a half angle of about 27 degrees, so the 9 cover the hemisphere.
static const float kConeTan = 0.5f;
static const uint kRingConeCount = 8;
static const float kRingPolarAngle = 0.9f;
// Cosine weights
static const float kAxisWeight = 1.0f;
static const float kRingWeight = 0.62161f; // cos(kRingPolarAngle)
static const float kInvTotalWeight = 1.0f / (kAxisWeight + kRingConeCount * kRingWeight);

// The volume is clamped to the band, so a sample there only says the surface is at least that far away.
// Slightly under the band to allow for filtering error.
static const float kBandFraction = 0.98f;

float SampleGlobal(float3 WorldPos)
{
    const float3 UVW = (WorldPos - c_Uniforms.VolumeMin) / (c_Uniforms.VolumeMax - c_Uniforms.VolumeMin);
    return t_tex3d_f1[c_Uniforms.VolumeTexture].SampleLevel(s_LinearClamp, UVW, 0.0f);
}

bool InsideVolume(float3 WorldPos)
{
    return all(WorldPos >= c_Uniforms.VolumeMin) && all(WorldPos <= c_Uniforms.VolumeMax);
}

// Builds a tangent frame around a unit normal (Duff et al. 2017)
void BuildBasis(float3 N, out float3 T, out float3 B)
{
    const float Sign = N.z >= 0.0f ? 1.0f : -1.0f;
    const float A = -1.0f / (Sign + N.z);
    const float C = N.x * N.y * A;
    T = float3(1.0f + Sign * N.x * N.x * A, Sign * C, -Sign * N.x);
    B = float3(C, Sign + N.y * N.y * A, -N.y);
}

// 1 when nothing intersects the cone, falling to 0 as the nearest surface approaches the cone axis
float ConeVisibility(float3 Origin, float3 Direction)
{
    const float BandLimit = c_Uniforms.Band * kBandFraction;

    float Visibility = 1.0f;
    float T = c_Uniforms.FirstStep;
    for (uint StepIt = 0; StepIt < c_Uniforms.StepsPerCone; StepIt++)
    {
        const float3 SamplePos = Origin + Direction * T;
        if (!InsideVolume(SamplePos))
            break;

        const float Distance = SampleGlobal(SamplePos);
        if (Distance < BandLimit)
        {
            Visibility = min(Visibility, saturate(Distance / (T * kConeTan)));
        }

        T *= c_Uniforms.StepScale;
    }

    return Visibility;
}

float DistanceFieldAO(uint2 PixelCoord)
{
    const float Depth = t_tex2d_f4[c_Uniforms.SceneDepthTexture].Load(int3(PixelCoord, 0)).r;
    if (Depth >= 1.0f)
        return 1.0f;

    const float2 UV = (float2(PixelCoord) + 0.5f) / float2(c_Uniforms.OutputSize);
    const float4 WorldH = mul(c_Uniforms.InvViewProjection, float4(UV.x * 2.0f - 1.0f, 1.0f - UV.y * 2.0f, Depth, 1.0f));
    const float3 WorldPos = WorldH.xyz / WorldH.w;

    if (!InsideVolume(WorldPos))
        return 1.0f;

    const float3 N = normalize(t_tex2d_f4[c_Uniforms.SceneNormalTexture].Load(int3(PixelCoord, 0)).xyz);
    const float3 Origin = WorldPos + N * c_Uniforms.NormalBias;

    float3 T, B;
    BuildBasis(N, T, B);

    float RingSin, RingCos;
    sincos(kRingPolarAngle, RingSin, RingCos);

    float Occlusion = kAxisWeight * ConeVisibility(Origin, N);

    [unroll]
    for (uint ConeIt = 0; ConeIt < kRingConeCount; ConeIt++)
    {
        float AzimuthSin, AzimuthCos;
        sincos(ConeIt * (2.0f * PI / kRingConeCount), AzimuthSin, AzimuthCos);

        const float3 Direction = (T * AzimuthCos + B * AzimuthSin) * RingSin + N * RingCos;
        Occlusion += kRingWeight * ConeVisibility(Origin, Direction);
    }

    return pow(Occlusion * kInvTotalWeight, c_Uniforms.Power);
}

[NumThreads(8, 8, 1)]
void main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    const uint2 PixelCoord = DispatchThreadID.xy;
    if (any(PixelCoord >= c_Uniforms.OutputSize))
        return;

    u_tex2d_f1[c_Uniforms.OutputTexture][PixelCoord] = c_Uniforms.Enabled != 0 ? DistanceFieldAO(PixelCoord) : 1.0f;
}

#endif // #ifdef _CS
