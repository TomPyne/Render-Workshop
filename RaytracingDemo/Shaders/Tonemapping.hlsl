#ifdef _PS

#include "ScreenPassShared.h"

struct Uniforms
{
    uint InputTexture;
    float WhitePoint;
    float ExposureBias;
    float __Pad; 
};

ConstantBuffer<Uniforms> c_G : register(b1);
Texture2D<float4> t_tex2d_f4[8192] : register(t0, space0);
SamplerState ClampedSampler : register(s1);

struct PS_OUTPUT
{
    float4 Color : SV_TARGET0;
};

#if U2TONEMAPPER

static const float A = 0.15f;
static const float B = 0.50f;
static const float C = 0.10f;
static const float D = 0.20f;
static const float E = 0.02f;
static const float F = 0.30f;

float3 U2Tonemap(float3 X)
{
    return ((X*(A*X+C*B)+D*E)/(X*(A*X+B)+D*F))-E/F;
}

void main(in PS_INPUT Input, out PS_OUTPUT Output)
{
    float3 InputColor = t_tex2d_f4[c_G.InputTexture].SampleLevel(ClampedSampler, Input.UV, 0u).rgb;
    
    float3 Exposed = U2Tonemap(c_G.ExposureBias * InputColor);

    float3 WhiteScale = U2Tonemap(c_G.WhitePoint.rrr); // Could be calculated on CPU

    Exposed /= WhiteScale;

    float3 OutColor = pow(Exposed, 1.0f / 2.2f);
    Output.Color = float4(OutColor,1);
}

#endif

#if NOTONEMAPPER
void main(in PS_INPUT Input, out PS_OUTPUT Output)
{
    float3 InputColor = t_tex2d_f4[c_G.InputTexture].SampleLevel(ClampedSampler, Input.UV, 0u).rgb;
    Output.Color = float4(InputColor,1);
}
#endif

#endif