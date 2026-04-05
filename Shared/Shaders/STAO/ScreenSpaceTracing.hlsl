struct ConstantData
{
    float4x4 Projection;
    float4x4 InverseProjection;
    row_major float4x4 View;

    uint DepthTextureIndex;
    uint NormalTextureIndex;
    uint OutputTextureIndex;
    float __pad0;

    float2 ViewportSizeRcp;
    uint2 ViewportSize;

    float Thickness;
	float MaxDistance;
	float MaxSteps;
	float Stride;
	
	float2 DepthProjection;
	float Jitter;
	float NearPlaneZ;
};
ConstantBuffer<ConstantData> c_G : register(CBV_SLOT);

Texture2D<float> t_tex2d_f1[8192] : register(t0, space0); // Depth 
Texture2D<float4> t_tex2d_f4[8192] : register(t0, space1); // Normal
RWTexture2D<float4> u_tex2d_f4[8192] : register(u0, space0); // Output

static const float Pi = 3.141593;

float LinearDepth(float Depth)
{
	return c_G.DepthProjection.y / (Depth - c_G.DepthProjection.x);
}

float3 GetViewPosFromScreen(float4x4 InverseProjection, float2 NDC, float DepthClip)
{
    float4 Projected = float4(NDC, DepthClip, 1.0f);
    float4 Unprojected = mul(InverseProjection, Projected);
    return Unprojected.xyz / Unprojected.w;
}

float3 GetViewPosFromScreen(float2 Pixel)
{
    float Depth = (t_tex2d_f1[c_G.DepthTextureIndex].Load(uint3(Pixel, 0)));
    float2 NDC = (Pixel * c_G.ViewportSizeRcp) * 2.0f - 1.0f;
    NDC.y = -NDC.y;
    return GetViewPosFromScreen(c_G.InverseProjection, NDC.xy, Depth);
}

float Random(float3 Seed)
{
    return frac(sin(dot(Seed.xyz, float3(12.9898,78.233, 128.943)))* 43758.5453123);
}

float3 GetRandomHemiSphere(uint3 Seed)
{
    const float Phi = Random(float3(Seed % 16)) * 2.0f * Pi;
    const float Theta = lerp(0.0f, Pi * 0.4f, Random(float3(Seed % 16)));
    const float SinTheta = sin(Theta);
    return float3(cos(Phi) * SinTheta, sin(Phi) * SinTheta, cos(Theta));
}

float3x3 ComputeBasisMatrix(float3 Normal)
{
    Normal = normalize(Normal);
    const float3 Up = abs(Normal.z) < 0.999 ? float3(0,0,1) : float3(1,0,0);
    const float3 Tangent = normalize(cross(Up, Normal));
    const float3 Bitangent = normalize(cross(Normal, Tangent));

    return transpose(float3x3(Tangent, Bitangent, Normal));
}

float SqrDist(float2 A, float2 B)
{
    const float2 Delta = B - A;
    return dot(Delta, Delta);
}

void Swap(inout float A, inout float B)
{
    const float T = B;
    B = A;
    A = T;
}

bool DepthIntersection(float Z, float MinZ, float MaxZ)
{
    return (MaxZ >= Z) && (MinZ - c_G.Thickness <= Z);
}

[NumThreads(8, 8, 1)]
void main(uint3 DispatchThreadId : SV_DispatchThreadID)
{
    if(any(DispatchThreadId.xy >= c_G.ViewportSize))
        return;

    const float3 RandomHemiTangentSpace = GetRandomHemiSphere(DispatchThreadId);    

    const float3 Normal = t_tex2d_f4[c_G.NormalTextureIndex].Load(uint3(DispatchThreadId.xy, 0)).xyz;
    const float3x3 TBN = ComputeBasisMatrix(Normal);

    float3 SampleDirWorldSpace = mul(TBN, RandomHemiTangentSpace);

    float3 DirectionViewSpace = normalize(mul((float3x3)c_G.View, SampleDirWorldSpace).xyz);

    float2 StartPixel = float2(DispatchThreadId.xy) + 0.5;

    float3 OriginViewSpace = GetViewPosFromScreen(StartPixel);
    OriginViewSpace += DirectionViewSpace * 0.01f;
    
    float RayLength = ((OriginViewSpace.z + DirectionViewSpace.z * c_G.MaxDistance) < c_G.NearPlaneZ) ? (c_G.NearPlaneZ - OriginViewSpace.z) / DirectionViewSpace.z : c_G.MaxDistance; // FLIPPED COMPARISON

    float3 EndPointViewSpace = OriginViewSpace + DirectionViewSpace * RayLength;

    float2 HitPixel = float2(-1, -1);

    float4 H0 = mul(c_G.Projection, float4(OriginViewSpace, 1.0f));
    float4 H1 = mul(c_G.Projection, float4(EndPointViewSpace, 1.0f));

    float K0 = 1.0f / H0.w;
    float K1 = 1.0f / H1.w;

    float3 Q0 = OriginViewSpace * K0;
    float3 Q1 = EndPointViewSpace * K1;

    float2 P0 = H0.xy * K0;
    float2 P1 = H1.xy * K1;

    P0 = P0 * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
    P1 = P1 * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);

    P0.xy *= c_G.ViewportSize;
    P1.xy *= c_G.ViewportSize;

    // L4

    P1 += float(SqrDist(P0, P1) < 0.0001f ? 0.01f : 0.0f).rr;

    float2 Delta = (P1 - P0);

    bool Permute = false;
    if(abs(Delta.x) < abs(Delta.y))
    {
        Permute = true;
        Delta = Delta.yx;
        P0 = P0.yx;
        P1 = P1.yx;
    }

    float StepDir = sign(Delta.x);
    float InvDx = StepDir / Delta.x;

    float3 DerivQ = (Q1 - Q0) * InvDx;
    float DerivK = (K1 - K0) * InvDx;
    float2 DerivP = float2(StepDir, Delta.y * InvDx);

    DerivP *= c_G.Stride;
    DerivQ *= c_G.Stride;
    DerivK *= c_G.Stride;
    P0 += DerivP * c_G.Jitter;
    Q0 += DerivQ * c_G.Jitter;
    K0 += DerivK * c_G.Jitter;

    float PrevZMaxEst = OriginViewSpace.z;

    float2 P = P0;
    float3 Q = Q0;
    float K = K0;
    float StepCount = 0.0f;
    float RayZMax = PrevZMaxEst;
    float RayZMin = PrevZMaxEst;
    float SceneZMax = RayZMax + 1e4;

    float End = P1.x * StepDir;
    
    for(; ((P.x * StepDir) <= End) && (StepCount < c_G.MaxSteps) && !DepthIntersection(SceneZMax, RayZMin, RayZMax) && (SceneZMax != 0.0f); P += DerivP, Q.z += DerivQ.z, K += DerivK, StepCount += 1.0f)
    {
        HitPixel = Permute ? P.yx : P;

        RayZMin = PrevZMaxEst;
        RayZMax = (DerivQ.z * 0.5f + Q.z) / (DerivK * 0.5f + K);
        PrevZMaxEst = RayZMax;        
        
        if(RayZMin > RayZMax)
        {
            Swap(RayZMin, RayZMax);
        }

        SceneZMax = LinearDepth(t_tex2d_f1[c_G.DepthTextureIndex].Load(int3(HitPixel, 0)));
    }

    Q.xy += DerivQ.xy * StepCount;
    float3 HitPoint = Q * (1.0f / K);
    bool Hit = (RayZMax >= SceneZMax - c_G.Thickness) && (RayZMin <= SceneZMax);

    u_tex2d_f4[c_G.OutputTextureIndex][DispatchThreadId.xy] = float4(DepthIntersection(SceneZMax, RayZMin, RayZMax) ? 0.0f.rrr : 1.0f.rrr, 1);
}