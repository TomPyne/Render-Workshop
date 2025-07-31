struct RayPayload
{
    float RayHitT;
};

#ifdef _RGS

static const float FLT_MAX = asfloat(0x7F7FFFFF);

RaytracingAccelerationStructure t_accel : register(t0, space0);
Texture2D<float> t_tex2d_f1[8192] : register(t1, space0); // Depth
RWTexture2D<float> u_tex2d_f1[8192] : register(u0, space0); // Shadow Buffer
RWTexture2D<float2> u_tex2d_f2[8192] : register(u0, space1); // Shadow History Buffer

struct Uniforms
{
    float4x4 CamToWorld;

    float3 SunDirection;
    float SunSoftAngle;

    float2 ScreenResolution;
    uint SceneDepthTextureIndex;
    uint SceneShadowTextureIndex;

    uint SceneShadowHistoryTextureIndex;
    float Time;
    float AccumFrames;
    float __pad0;
};

ConstantBuffer<Uniforms> c_Uniforms : register(b0);

float Random(float3 Seed)
{
    return frac(sin(dot(Seed.xyz, float3(12.9898,78.233, 128.943)))* 43758.5453123);
}

float3 GetWorldPos(float4x4 CamToWorld, float2 ScreenPos, float Depth)
{
    float4 Unprojected = mul(CamToWorld, float4(ScreenPos, Depth, 1));
    return Unprojected.xyz / Unprojected.w;
}

[shader("raygeneration")]
void RayGen()
{
    uint2 DTid = DispatchRaysIndex().xy;
    float2 Pixel = DTid.xy + 0.5;

    // Screen pos for ray
    float2 ScreenPos = Pixel / c_Uniforms.ScreenResolution * 2.0f - 1.0f;
    ScreenPos.y = -ScreenPos.y;

    float SceneDepth = t_tex2d_f1[c_Uniforms.SceneDepthTextureIndex].Load(int3(Pixel, 0));

    float3 WorldPos = GetWorldPos(c_Uniforms.CamToWorld, ScreenPos, SceneDepth);

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

    RayDesc Ray = { WorldPos, 0.1f, RayDirection, FLT_MAX };
    RayPayload Payload = { FLT_MAX };

    TraceRay(t_accel, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, ~0, 0, 1, 0, Ray, Payload);

    const float Shadow = Payload.RayHitT < FLT_MAX ? 1.0f : 0.0f;    

    // Accumulate history
    //float2 History = u_tex2d_f2[c_Uniforms.SceneShadowHistoryTextureIndex][DTid];
    //float Smoothing = 2.0f / ((History.y * 255.0f) + 1.0f);
    //loat AccumulatedShadow = (Shadow * Smoothing) + (History.x) * (1.0f - Smoothing);
    //u_tex2d_f2[c_Uniforms.SceneShadowHistoryTextureIndex][DTid] = float2(AccumulatedShadow, (History.y + 1.0f) / 255.0f);

    //u_tex2d_f2[c_Uniforms.SceneShadowHistoryTextureIndex][DTid] = float2(lerp(History.x, Shadow, saturate(1.1f - History.y)), History.y += (2.0f / 255.0f));
    float History = u_tex2d_f1[c_Uniforms.SceneShadowTextureIndex][DTid];
    u_tex2d_f1[c_Uniforms.SceneShadowTextureIndex][DTid] = lerp(Shadow, History, saturate(pow(c_Uniforms.AccumFrames * 0.001f, 0.01f)));
}

#endif

#ifdef _RMS
[shader("miss")]
void Miss(inout RayPayload Payload)
{
    Payload.RayHitT = 0;
}
#endif