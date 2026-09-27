#ifdef _CS

static const float FLT_MAX = asfloat(0x7F7FFFFF);

struct Uniforms_s
{
    float4x4 CamToWorld;
    
    float3 SunDirection;
    float SunSoftAngle;

    float2 ScreenResolution;
    uint SceneDepthTexture;
    uint SceneShadowTexture;

    float Time;
    float3 __Pad;
};

ConstantBuffer<Uniforms_s> c_Uniforms : register(b0);

RaytracingAccelerationStructure t_accel : register(t0, space0);
Texture2D<float> t_tex2d_f1[8192] : register(t1, space0); // Depth
RWTexture2D<float> u_tex2d_f1[8192] : register(u0, space0); // Shadow Buffer

float Random(float3 Seed)
{
    return frac(sin(dot(Seed.xyz, float3(12.9898,78.233, 128.943)))* 43758.5453123);
}

float3 GetWorldPos(float2 ScreenPos, float Depth)
{
    float4 Unprojected = mul(c_Uniforms.CamToWorld, float4(ScreenPos, Depth, 1));
    return Unprojected.xyz / Unprojected.w;
}

[NumThreads(8, 8, 1)]
void main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    float2 Pixel = float2(DispatchThreadID.xy) + 0.5f;

    float SceneDepth = t_tex2d_f1[c_Uniforms.SceneDepthTexture].Load(int3(Pixel, 0));

    float2 ScreenPos = Pixel / c_Uniforms.ScreenResolution * 2.0f - 1.0f;
    ScreenPos.y = -ScreenPos.y;
    float3 WorldPos = GetWorldPos(ScreenPos, SceneDepth);

    float3 Axis = c_Uniforms.SunDirection;

    float R1 = saturate(Random(WorldPos + c_Uniforms.Time.xxx));
    float R2 = saturate(Random(WorldPos * 2.0f + c_Uniforms.Time.xxx));

    float3 Ortho1 = normalize(cross(Axis, float3(0, 0, 1)));
    float3 Ortho2 = cross(Axis, Ortho1);
    float Theta = acos(1 - R1 * (1 - cos(c_Uniforms.SunSoftAngle)));
    float Phi = R2 * 6.28318530718;

    float SinTheta = sin(Theta);
    float CosTheta = cos(Theta);

    float SinThetaCosPhi = SinTheta * cos(Phi);
    float SinPhiSinTheta = SinTheta * sin(Phi);
    
    float3 RayDirection = float3(
        SinThetaCosPhi * Ortho1.x + SinPhiSinTheta * Ortho2.x + CosTheta * Axis.x,
        SinThetaCosPhi * Ortho1.y + SinPhiSinTheta * Ortho2.y + CosTheta * Axis.y,
        SinThetaCosPhi * Ortho1.z + SinPhiSinTheta * Ortho2.z + CosTheta * Axis.z
    );

    // https://devblogs.microsoft.com/directx/dxr-1-1/
    // https://github.com/microsoft/DirectX-Specs/blob/master/d3d/Raytracing.md#inline-raytracing
    RayQuery<RAY_FLAG_CULL_NON_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> ShadowQuery;

    RayDesc Ray = { WorldPos + RayDirection * 0.1f, 0.1f, RayDirection, FLT_MAX };

    ShadowQuery.TraceRayInline(t_accel, RAY_FLAG_NONE, 0xFF, Ray);
    ShadowQuery.Proceed();

    u_tex2d_f1[c_Uniforms.SceneShadowTexture][Pixel] = ShadowQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT ? 0.0f : 1.0f;
}

#endif // #ifdef _CS