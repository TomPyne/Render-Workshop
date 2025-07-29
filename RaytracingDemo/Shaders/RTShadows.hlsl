struct RayPayload
{
    float RayHitT;
};

#ifdef _RGS

static const float FLT_MAX = asfloat(0x7F7FFFFF);

RaytracingAccelerationStructure t_accel : register(t0, space0);
Texture2D<float> t_tex2d_f1[8192] : register(t1, space0); // Depth
RWTexture2D<float> u_tex2d_f1[8192] : register(u1, space0); // Shadow Buffer

struct Uniforms
{
    float4x4 CamToWorld;

    float3 SunDirection;
    float __pad0;

    float2 ScreenResolution;
    uint SceneDepthTextureIndex;
    uint SceneShadowTextureIndex;
};

ConstantBuffer<Uniforms> c_Uniforms : register(b0);

[shader("raygeneration")]
void RayGen()
{
    uint2 DTid = DispatchRaysIndex().xy;
    float2 Pixel = DTid.xy + 0.5;

    // Screen pos for ray
    float2 ScreenPos = Pixel / c_Uniforms.ScreenResolution * 2.0f - 1.0f;
    ScreenPos.y = -ScreenPos.y;

    float SceneDepth = t_tex2d_f1[c_Uniforms.SceneDepthTextureIndex].Load(int3(Pixel, 0));

    float4 Unprojected = mul(c_Uniforms.CamToWorld, float4(ScreenPos, SceneDepth, 1));
    float3 WorldPos = Unprojected.xyz / Unprojected.w;

    RayDesc Ray = { WorldPos, 0.1f, c_Uniforms.SunDirection, FLT_MAX };
    RayPayload Payload = { FLT_MAX };

    TraceRay(t_accel, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, ~0, 0, 1, 0, Ray, Payload);

    u_tex2d_f1[c_Uniforms.SceneShadowTextureIndex][DispatchRaysIndex().xy] = Payload.RayHitT < FLT_MAX ? 1.0f : 0.0f;
}

#endif

#ifdef _RMS
[shader("miss")]
void Miss(inout RayPayload Payload)
{
    Payload.RayHitT = 0;
}
#endif