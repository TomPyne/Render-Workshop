struct ConstantData
{
    row_major float4x4 Projection;
    float4x4 InverseProjection;
    float4x4 View;

    uint DepthTextureIndex;
    uint NormalTextureIndex;
    uint OutputTextureIndex;
    float __pad0;

    float2 ViewportSizeRcp;
    uint2 ViewportSize;
};
ConstantBuffer<ConstantData> c_G : register(CBV_SLOT);

Texture2D<float> t_tex2d_f1[8192] : register(t0, space0); // Depth 
Texture2D<float4> t_tex2d_f4[8192] : register(t0, space1); // Normal
RWTexture2D<float4> u_tex2d_f4[8192] : register(u0, space0); // Output

static const float Pi = 3.141593;

float3 GetViewPosFromScreen(float4x4 InverseProjection, float2 NDC, float DepthClip)
{
    float4 Projected = float4(NDC, DepthClip, 1.0f);
    float4 Unprojected = mul(InverseProjection, Projected);
    return Unprojected.xyz / Unprojected.w;
}

float3 GetViewPosFromScreen(float2 Pixel)
{
    float Depth = t_tex2d_f1[c_G.DepthTextureIndex].Load(uint3(Pixel, 0));
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
    const float Theta = Random(float3(Seed % 16));
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

static const float Thickness = 0.5f;
static const float NearPlaneZ = 0.1f;
static const float FarPlaneZ = 1000.0f;
static const float ProjectionA = FarPlaneZ / (FarPlaneZ - NearPlaneZ);
static const float ProjectionB = (-FarPlaneZ * NearPlaneZ) / (FarPlaneZ - NearPlaneZ);

bool DepthIntersection(float Z, float MinZ, float MaxZ)
{
    return (MaxZ >= Z) && (MinZ - Thickness <= Z);
}

float LinearDepth(float Depth)
{
	return ProjectionB / (Depth - ProjectionA);
}

[NumThreads(8, 8, 1)]
void main(uint3 DispatchThreadId : SV_DispatchThreadID)
{
    if(any(DispatchThreadId.xy >= c_G.ViewportSize))
        return;

    const float MaxDistance = 100.0f;
    const float Resolution = 0.3f;
    const float MaxSteps = 30.0f;
    const float NearPlane = 0.1f;
    const float Stride = 1.0f;
    const float Jitter = 0.0f;

    const float3 RandomHemiTangentSpace = GetRandomHemiSphere(DispatchThreadId);    

    const float3 Normal = t_tex2d_f4[c_G.NormalTextureIndex].Load(uint3(DispatchThreadId.xy, 0)).xyz;
    const float3x3 TBN = ComputeBasisMatrix(Normal);

    float3 SampleDirWorldSpace = Normal;//mul(TBN, RandomHemiTangentSpace);

    float2 StartPixel = float2(DispatchThreadId.xy) + 0.5;

    float3 OriginViewSpace = GetViewPosFromScreen(StartPixel);
    float3 DirectionViewSpace = mul(c_G.View, float4(SampleDirWorldSpace, 0.0f)).xyz;

    float RayLength = ((OriginViewSpace.z + DirectionViewSpace.z * MaxDistance) < NearPlane) ? (NearPlane - OriginViewSpace.z) / DirectionViewSpace.z : MaxDistance; // FLIPPED COMPARISON
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

    float2 Delta = P1 - P0;

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

    DerivP *= Stride;
    DerivQ *= Stride;
    DerivK *= Stride;
    P0 += DerivP * Jitter;
    Q0 += DerivQ * Jitter;
    K0 += DerivK * Jitter;

    float PrevZMaxEst = OriginViewSpace.z;

    float3 Q = Q0;
    float K = K0;
    float StepCount = 0.0f;
    float RayZMax = PrevZMaxEst;
    float RayZMin = PrevZMaxEst;
    float SceneZMax = RayZMax + 1e4;

    float End = P1.x * StepDir;
    
    for(float2 P = P0; ((P.x * StepDir) <= End) && (StepCount < MaxSteps) && DepthIntersection(SceneZMax, RayZMin, RayZMax) && (SceneZMax != 0.0f); P += DerivP, Q.z += DerivQ.z, K += DerivK, StepCount += 1.0f)
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
    bool Hit = (RayZMax >= SceneZMax - Thickness) && (RayZMin <= SceneZMax);  //all((abs(HitPixel - c_G.ViewportSize * 0.5f)) <= c_G.ViewportSize * 0.5f);

    u_tex2d_f4[c_G.OutputTextureIndex][DispatchThreadId.xy] = float4(DepthIntersection(SceneZMax, RayZMin, RayZMax) ? 0.0f : 1.0f, 0, 0, 1);
    // float2 Test0 = H0.xy * K0;;
    // float2 Test = Test0;// * 0.5f + 0.5f;
    u_tex2d_f4[c_G.OutputTextureIndex][DispatchThreadId.xy] = float4(P0,0, 1);
    return;

#if 0
    ////

    float3 PositionFrom = GetViewPosFromScreen(StartPixel);
    float3 Direction = normalize(PositionFrom);
    
    float3 SampleDirView = mul(c_G.View, float4(SampleDirWorldSpace, 1.0f)).xyz;

    float3 PositionTo = PositionFrom + SampleDirView * MaxDistance;
    
    float4 EndPixel = mul(c_G.Projection, float4(PositionTo, 1.0f));
    EndPixel.xyz /= EndPixel.w;

    //EndPixel.xy = EndPixel.xy * 0.5f + 0.5f;
    EndPixel.xy *= c_G.ViewportSize;

    float DeltaX = EndPixel.x - StartPixel.x;
    float DeltaY = EndPixel.y - StartPixel.y;

    bool UseX = abs(DeltaX) >= abs(DeltaY);
    float Delta = UseX ? abs(DeltaX) : abs(DeltaY);
    Delta *= saturate(Resolution);

    float2 Increment = float2(DeltaX, DeltaY) / max(Delta, 0.001f);

    float Search0 = 0.0f;
    float Search1 = 0.0f;
    bool Hit0 = false;
    bool Hit1 = false;

    float ViewDistance = PositionFrom.z;
    float TraceDepth = Thickness;

    float2 TracePixel = StartPixel;

    for(int i = 0; i < int(Delta); i++)
    {
        TracePixel += Increment;
        PositionTo = GetViewPosFromScreen(TracePixel);
        Search1 = UseX ? (TracePixel.x - StartPixel.x) / DeltaX : (TracePixel.y - StartPixel.y) / DeltaY;

        ViewDistance = (PositionFrom.y * PositionTo.y) / lerp(PositionTo.y, PositionFrom.y, Search1);

        TraceDepth = ViewDistance - PositionTo.y;

        if(TraceDepth > 0 && TraceDepth < Thickness)
        {
            Hit0 = true;
            break;
        }
        else
        {
            Search0 = Search1;
        }

        Search1 = Search0 + ((Search1 - Search0));
    }

    if(Hit0)
    {
        for(int i = 0; i < Steps; i++)
        {
            TracePixel = lerp(StartPixel, EndPixel.xy, Search1);
            PositionTo = GetViewPosFromScreen(TracePixel);

            ViewDistance = (PositionFrom.y * PositionTo.y) / lerp(PositionTo.y, PositionFrom.y, Search1);
            TraceDepth = ViewDistance - PositionTo.y;

            if(TraceDepth > 0 && TraceDepth < Thickness)
            {
                Hit1 = true;
                Search1 = Search0 + ((Search1 - Search0) / 2.0f);
            }
            else
            {
                float Temp = Search1;
                Search1 = Search1 + ((Search1 - Search0) / 2.0f);
                Search0 = Temp;
            }
        }
    }

    float Visibility = 0.0f;
    if(Hit1 && all(int2(TracePixel) >= 0) && all(int2(TracePixel) < c_G.ViewportSize))
    {
        //Visibility *= PositionTo.w;
        Visibility *= 1.0f - max(dot(Direction, SampleDirView), 0.0f);
        Visibility *= 1.0f - saturate(TraceDepth /  Thickness);
        Visibility *= 1.0f - saturate(length(PositionTo - PositionFrom) / MaxDistance);

        Visibility = saturate(Visibility);
    }

    u_tex2d_f4[c_G.OutputTextureIndex][DispatchThreadId.xy] = float4(Visibility.rrr, 1.0f);
#endif
}