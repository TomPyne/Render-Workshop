#ifdef _CS

// Must match the kSpatialWeight constants in ShadowDenoiser.cpp
#define WEIGHT_PLANE 1
#define WEIGHT_NORMAL 2
#define WEIGHT_VARIANCE 4

struct Uniforms_s
{
    float4x4 CamToWorld;

    uint2 ViewportSize;
    float2 InvViewportSize;

    uint InShadowTexture;
    uint OutShadowTexture;
    uint SceneDepthTexture;
    uint SceneNormalTexture;

    uint StepSize;
    float DepthSigma;
    float NormalPower;
    float VarianceSigma;

    uint WeightFlags;
    float3 __Pad;
};

ConstantBuffer<Uniforms_s> c_Uniforms : register(b0);

Texture2D<float> t_tex2d_f1[8192] : register(t1, space0); // Depth
Texture2D<float2> t_tex2d_f2[8192] : register(t1, space1); // Shadow + variance
Texture2D<float4> t_tex2d_f4[8192] : register(t1, space2); // Normal
RWTexture2D<float2> u_tex2d_f2[8192] : register(u0, space0); // Shadow + variance out

static const float kEpsilon = 1e-6f;
// B-spline weights at tap offsets 0, 1 and 2
static const float kKernel[3] = { 3.0f / 8.0f, 1.0f / 4.0f, 1.0f / 16.0f };
static const float kGaussian3x3[2] = { 1.0f / 2.0f, 1.0f / 4.0f };

// xyz is the world position, w the view depth
float4 GetWorldPosAndLinearDepth(uint2 PixelCoord, float SceneDepth)
{
    const float2 UV = (float2(PixelCoord) + 0.5f) * c_Uniforms.InvViewportSize;
    const float2 ScreenPos = float2(UV.x, 1.0f - UV.y) * 2.0f - 1.0f;

    // The unprojected w is 1 / clip w, and clip w is view depth
    const float4 Unprojected = mul(c_Uniforms.CamToWorld, float4(ScreenPos, SceneDepth, 1.0f));
    return float4(Unprojected.xyz / Unprojected.w, 1.0f / Unprojected.w);
}

// A single pixel's variance is too noisy to steer the variance weight, so it's blurred first
float PrefilterVariance(uint2 PixelCoord)
{
    float Sum = 0.0f;
    float WeightSum = 0.0f;

    for (int Y = -1; Y <= 1; Y++)
    {
        for (int X = -1; X <= 1; X++)
        {
            const int2 Coord = int2(PixelCoord) + int2(X, Y);
            if (any(Coord < 0) || any(Coord >= int2(c_Uniforms.ViewportSize)))
                continue;

            const float Weight = kGaussian3x3[abs(X)] * kGaussian3x3[abs(Y)];
            Sum += Weight * t_tex2d_f2[c_Uniforms.InShadowTexture][Coord].y;
            WeightSum += Weight;
        }
    }

    return Sum / WeightSum;
}

[NumThreads(8, 8, 1)]
void main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    const uint2 PixelCoord = DispatchThreadID.xy;
    if (any(PixelCoord >= c_Uniforms.ViewportSize))
        return;

    const float CentreDepth = t_tex2d_f1[c_Uniforms.SceneDepthTexture][PixelCoord];
    if (CentreDepth >= 1.0f)
    {
        u_tex2d_f2[c_Uniforms.OutShadowTexture][PixelCoord] = float2(1.0f, 0.0f);
        return;
    }

    const bool UsePlane = (c_Uniforms.WeightFlags & WEIGHT_PLANE) != 0;
    const bool UseNormal = (c_Uniforms.WeightFlags & WEIGHT_NORMAL) != 0;
    const bool UseVariance = (c_Uniforms.WeightFlags & WEIGHT_VARIANCE) != 0;

    const float4 CentrePos = GetWorldPosAndLinearDepth(PixelCoord, CentreDepth);
    const float3 CentreNormal = normalize(t_tex2d_f4[c_Uniforms.SceneNormalTexture][PixelCoord].xyz);
    const float CentreShadow = t_tex2d_f2[c_Uniforms.InShadowTexture][PixelCoord].x;
    const float VarianceScale = UseVariance ? c_Uniforms.VarianceSigma * sqrt(PrefilterVariance(PixelCoord)) + kEpsilon : 1.0f;

    float WeightSum = 0.0f;
    float ShadowSum = 0.0f;
    float VarianceSum = 0.0f;

    for (int Y = -2; Y <= 2; Y++)
    {
        for (int X = -2; X <= 2; X++)
        {
            const int2 Coord = int2(PixelCoord) + int2(X, Y) * int(c_Uniforms.StepSize);
            if (any(Coord < 0) || any(Coord >= int2(c_Uniforms.ViewportSize)))
                continue;

            const float Depth = t_tex2d_f1[c_Uniforms.SceneDepthTexture][Coord];
            if (Depth >= 1.0f)
                continue;

            const float2 Sample = t_tex2d_f2[c_Uniforms.InShadowTexture][Coord];

            float Weight = kKernel[abs(X)] * kKernel[abs(Y)];

            if (UsePlane)
            {
                const float3 Pos = GetWorldPosAndLinearDepth(Coord, Depth).xyz;
                const float PlaneDistance = abs(dot(CentreNormal, Pos - CentrePos.xyz)) / CentrePos.w;
                Weight *= exp(-PlaneDistance / c_Uniforms.DepthSigma);
            }

            if (UseNormal)
            {
                const float3 Normal = normalize(t_tex2d_f4[c_Uniforms.SceneNormalTexture][Coord].xyz);
                Weight *= pow(saturate(dot(CentreNormal, Normal)), c_Uniforms.NormalPower);
            }

            if (UseVariance)
            {
                Weight *= exp(-abs(CentreShadow - Sample.x) / VarianceScale);
            }

            WeightSum += Weight;
            ShadowSum += Weight * Sample.x;
            VarianceSum += Weight * Weight * Sample.y;
        }
    }

    // The centre tap always contributes, so WeightSum is never 0
    u_tex2d_f2[c_Uniforms.OutShadowTexture][PixelCoord] = float2(ShadowSum / WeightSum, VarianceSum / (WeightSum * WeightSum));
}

#endif // #ifdef _CS
