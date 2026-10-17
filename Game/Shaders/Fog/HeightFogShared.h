#pragma once

struct HeightFogParams_s
{
    float Density;
    float HeightFalloff;
    float BaseHeight;
    float StartDistance;
    float MaxOpacity;
};

// Exponents are clamped so a ray ending far below the base height saturates instead of producing inf - inf
float HeightFogDensityScale(float Height, float HeightFalloff, float BaseHeight)
{
    return exp(min(-HeightFalloff * (Height - BaseHeight), 80.0f));
}

// Density is Density * exp(-HeightFalloff * (h - BaseHeight)), integrated analytically from StartDistance to WorldPos
float HeightFogTransmittance(float3 CameraPos, float3 WorldPos, HeightFogParams_s Params)
{
    const float3 V = WorldPos - CameraPos;
    const float Distance = length(V);
    if (Distance <= Params.StartDistance)
        return 1.0f;

    const float3 Dir = V / Distance;
    const float3 Origin = CameraPos + Dir * Params.StartDistance;
    const float Length = Distance - Params.StartDistance;

    const float OriginScale = HeightFogDensityScale(Origin.y, Params.HeightFalloff, Params.BaseHeight);
    const float X = Params.HeightFalloff * Dir.y * Length;

    // (1 - exp(-X)) / X, which tends to 1 for flat rays or uniform fog
    float Ratio;
    if (abs(X) > 0.01f)
    {
        const float EndScale = HeightFogDensityScale(WorldPos.y, Params.HeightFalloff, Params.BaseHeight);
        Ratio = (OriginScale - EndScale) / X;
    }
    else
    {
        Ratio = OriginScale * (1.0f - X * 0.5f + X * X / 6.0f);
    }

    const float OpticalDepth = Params.Density * Length * Ratio;
    return max(exp(-OpticalDepth), 1.0f - Params.MaxOpacity);
}

float HenyeyGreensteinPhase(float G, float CosTheta)
{
    const float G2 = G * G;
    const float Denom = 1.0f + G2 - 2.0f * G * CosTheta;
    return (1.0f - G2) / (4.0f * 3.14159265f * Denom * sqrt(Denom));
}
