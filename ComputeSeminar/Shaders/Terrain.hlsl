#include "Samplers.h"
#include "View.h"

ConstantBuffer<ViewData> c_View : register(b0);

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD;
    float3 WorldPosition : WORLDPOS;
};

cbuffer meshBuf : register(b1)
{
    float2 TileWorldPos;
    float2 TileScale;
    uint NoiseTex;
    uint NormalTex;
    float Height;
    float CellSize;
};

#if _BINDLESS
Texture2D<float> t_tex2d_float[512] : register(t0, space0);
Texture2D<float3> t_tex2d_float3[512] : register(t0, space1);
#else
Texture2D<float> NoiseTexture : register(t0);
#endif

#ifdef _VS

struct VS_INPUT
{
    float2 UV : TEXCOORD;
};

float SampleHeight(float2 UV)
{
#if _BINDLESS
    return t_tex2d_float[NoiseTex].SampleLevel(TrilinearSampler, UV, 0).r;
#else
    return NoiseTexture.SampleLevel(TrilinearSampler, UV, 0).r;
#endif
}

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;

    float height = SampleHeight(input.UV);

    output.WorldPosition = float3(input.UV.x * TileScale.x + TileWorldPos.x, height * Height, input.UV.y * TileScale.y + TileWorldPos.y);

    output.Position = mul(c_View.ViewProjectionMatrix, float4(output.WorldPosition, 1.0f));
    output.UV = input.UV;

    return output;
};

#endif

#ifdef _PS

float4 main(PS_INPUT input) : SV_Target0
{
#if _BINDLESS
    float3 normal = normalize(t_tex2d_float3[NormalTex].Sample(TrilinearSampler, input.UV).rgb);
#else
    float Noise = NoiseTexture.Sample(TrilinearSampler, input.UV).r;
#endif

    //float3 normal = normalize(input.Normal);
    float3 lightDir = normalize(float3(0.5f, 1.0f, 0.5f));
    float3 lightColor = float3(1.0f, 1.0f, 1.0f) * 0.8f;
    float ambientStrength = 0.5f;
    float3 viewDir = normalize(c_View.CameraPos - input.WorldPosition);
    float3 halfDir = normalize(lightDir + viewDir);

    float3 ambient  = ambientStrength * lightColor;
    float3 diff = max(dot(normal, lightDir), 0.0f) * lightColor;
    float3 spec = pow(max(dot(normal, halfDir), 0.0f), 0.0f) * lightColor * 0.0f;

    float3 terrainColor = lerp(float3(0.5f, 0.5f, 0.5f), float3(0.0f, 0.5f, 0.0f), normal.y);
    float3 finalColor = (ambient + diff + spec) * terrainColor;
    finalColor *= 8;
    finalColor = floor(finalColor);
    finalColor *= 0.125f;

    return float4(finalColor, 1.0f);
};

#endif