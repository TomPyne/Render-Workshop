#ifndef CBV_SLOT
#error Must define CBV_SLOT
#endif

struct Uniforms
{
    float4x4 ViewProjectionMatrix;

    float3 CameraPos;
    float PlanetRadius;

    float AtmosphereRadius;
    float3 SunDirection;

    float AtmosphereThicknessR;
    float AtmosphereThicknessM;
    float2 Pad;
};

ConstantBuffer<Uniforms> c_G : register(CBV_SLOT);

struct PixelInputs
{
    float4 SVPosition : SV_Position;
    float3 Direction : DIRECTION;
};

#if _VS

struct VertexInputs
{
    float3 Position : POSITION;
};

void main(in VertexInputs Input, out PixelInputs Output)
{   
    float3 LocalPos = Input.Position + c_G.CameraPos;
    Output.Direction = normalize(Input.Position);
    Output.SVPosition = mul(c_G.ViewProjectionMatrix, float4(LocalPos, 1.0f));
}

#endif

#if _PS

static const uint NumSamples = 16;
static const uint NumSamplesLight = 8;
static const float Pi = 3.141592654f;
static const float G = 0.76f;
static const float3 BetaR = float3(3.8e-6f, 13.5e-6f, 33.1e-6f);
static const float3 BetaM = float3(21e-6f, 21e-6f, 21e-6f);

bool QuadraticSolver(float A, float B, float C, out float2 X)
{
    if(B == 0.0f)
    {
        if(A == 0.0f)
        {
            return false;
        }
        X.x = 0.0f;
        X.y = sqrt(-C / A);
    }

    float D = B * B - 4.0f * A * C;

    if(D < 0.0f)
    {
        X = float2(0, 0);
        return false;
    }

    float Dr = sqrt(D);
    float Q = -0.5f * (B < 0.0f ? B - Dr : B + Dr);
    X.x = Q / A;
    X.y = C / Q;

    return true;
}

// ray-sphere intersection that assumes
// the sphere is centered at the origin.
// No intersection when result.x > result.y
bool  RaySphereIntersect(float3 Origin, float3 Direction, float Radius, out float2 T)
{
    T = float2(0.0f, 0.0f);
    float A = dot(Direction, Direction);
    float B = 2.0f * dot(Direction, Origin);
    float C = dot(Origin, Origin) - (Radius * Radius);
    if(!QuadraticSolver(A, B, C, T))
        return false;

    if(T.x > T.y)
        T = T.yx;

    return true;
}

float3 CalculateAtmosphereLight(float3 Origin, float3 Direction)
{
    float TMin = 0.0f;
    float TMax = 999999999999999.0f;

    float2 T;
    if(!RaySphereIntersect(Origin, Direction, c_G.AtmosphereRadius, T))
        return 0;

    if(T.y < 0.0f)
        return 0;

    if(T.x > TMin && T.x > 0)
        TMin = T.x;

    if(T.y < TMax)
        TMax = T.y;

    float SegmentLength = (TMax - TMin) / NumSamples;
    float TCurrent = TMin;
    float3 SumR = 0;
    float3 SumM = 0;

    float OpticalDepthR = 0;
    float OpticalDepthM = 0;

    float Mu = dot(Direction, c_G.SunDirection);
    float PhaseR = 3.0f / (16.0f * Pi) * (1.0f + Mu * Mu);
    float PhaseM = 3.0f / (8.0f * Pi) * ((1.0f - G * G) * (1.0f + Mu * Mu)) / ((2.0f + G * G) * pow(1.0f + G * G - 2.f * G * Mu, 1.5f));

    for(uint SampIt = 0; SampIt < NumSamples; SampIt++)
    {
        float3 SamplePosition = Origin + (TCurrent + SegmentLength * 0.5f) * Direction;
        float Height = length(SamplePosition) - c_G.PlanetRadius; // Why not just used Y Component?
        float HR = exp(-Height / c_G.AtmosphereThicknessR) * SegmentLength; // RCP also combine exps?
        float HM = exp(-Height / c_G.AtmosphereThicknessM) * SegmentLength;

        OpticalDepthR += HR;
        OpticalDepthM += HM;

        float2 TLight;
        RaySphereIntersect(SamplePosition, c_G.SunDirection, c_G.AtmosphereRadius, TLight); // Assumes intersection?
        float SegmentLengthLight = TLight.y / NumSamplesLight;
        float TCurrentLight = 0;
        float OpticalDepthLightR = 0.0f;
        float OpticalDepthLightM = 0.0f;
        uint LightSampIt = 0;
        for(; LightSampIt < NumSamplesLight; LightSampIt++)
        {
            float3 SamplePositionLight = SamplePosition + (TCurrentLight + SegmentLengthLight * 0.5f) * c_G.SunDirection;
            float HeightLight = length(SamplePositionLight) - c_G.PlanetRadius;
            if(HeightLight < 0)
                break;

            OpticalDepthLightR += exp(-HeightLight / HR) * SegmentLengthLight;
            OpticalDepthLightM += exp(-HeightLight / HM) * SegmentLengthLight;
            TCurrentLight += SegmentLengthLight;
        }

        if(LightSampIt == NumSamplesLight)
        {
            float3 Tau = BetaR * (OpticalDepthR + OpticalDepthLightR) + BetaM * 1.1f * (OpticalDepthM + OpticalDepthLightM);
            float3 Attenuation = exp(-Tau);
            SumR += Attenuation * HR;
            SumM += Attenuation * HM;
        }
        TCurrent += SegmentLength;
    }

    return SumR * BetaR * PhaseR + SumM * BetaM * PhaseM;
}

float4 main(in PixelInputs Input) : SV_TARGET
{   
    float3 Pos = /*(c_G.CameraPos / 1000.0f) + */float3(0, c_G.PlanetRadius + 1.0f,0.0f);
    float3 Color = CalculateAtmosphereLight(Pos, normalize(Input.Direction)) * 10.0f;
    return float4(Color, 1.0f);
}
#endif




