#ifdef __INTELLISENSE__
#define _VS
#define _MS
#define _PS
#endif // #ifdef __INTELLISENSE__


#ifdef _PS

#ifndef CBV_SLOT
#error Please define CBV_SLOT
#endif

#include "../ScreenPass/ScreenPassShared.h"

Texture2D<float4> t_tex2d_f4[8192] : register(t0, space0);
SamplerState ClampedSampler : register(s1);

struct PS_OUTPUT
{
    float4 Color : SV_TARGET0;
};

#if FILMICTONEMAPPER

struct Uniforms
{
    uint InputTexture;
    float ExposureBias;
    float WhitePoint;
    float __Pad; 
};

ConstantBuffer<Uniforms> c_G : register(CBV_SLOT);

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

#if ACESTONEMAPPER

struct Uniforms
{
    uint InputTexture;
    float ExposureBias;
    float2 __Pad; 
};

ConstantBuffer<Uniforms> c_G : register(CBV_SLOT);

static const float3x3 ACESInputMat =
{
    0.59719f, 0.35458f, 0.04823f,
    0.07600f, 0.90834f, 0.01566f,
    0.02840f, 0.13383f, 0.83777f
};

static const float3x3 ACESOutputMat =
{
     1.60475f, -0.53108f, -0.07367f,
    -0.10208f,  1.10813f, -0.00605f,
    -0.00327f, -0.07276f,  1.07602f
};

float3 RRTAndODTFit(float3 X)
{
    float3 a = X * (X + 0.0245786f) - 0.000090537f;
    float3 b = X * (0.983729f * X + 0.4329510f) + 0.238081f;
    return a / b;
}

float3 ACESFitted(float3 X)
{
    X = mul(ACESInputMat, X);
    X = RRTAndODTFit(X);
    X = mul(ACESOutputMat, X);
    return saturate(X);
}

void main(in PS_INPUT Input, out PS_OUTPUT Output)
{
    float3 InputColor = t_tex2d_f4[c_G.InputTexture].SampleLevel(ClampedSampler, Input.UV, 0u).rgb;

    float3 Exposed = ACESFitted(c_G.ExposureBias * InputColor);

    float3 OutColor = pow(Exposed, 1.0f / 2.2f);
    Output.Color = float4(OutColor,1);
}

#endif

#if NOTONEMAPPER

struct Uniforms
{
    uint InputTexture;
    float3 __Pad; 
};

ConstantBuffer<Uniforms> c_G : register(CBV_SLOT);

void main(in PS_INPUT Input, out PS_OUTPUT Output)
{
    float3 InputColor = t_tex2d_f4[c_G.InputTexture].SampleLevel(ClampedSampler, Input.UV, 0u).rgb;
    Output.Color = float4(InputColor,1);
}
#endif

#endif