struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD;
    float3 Normal : NORMAL;
};

cbuffer meshBuf : register(b1)
{
    float2 TileWorldPos;
    float2 TileScale;
    uint NoiseTex;
    float Height;
    float CellSize;
    float2 _Pad;
};

#if _BINDLESS
Texture2D<float> t_tex2d_float[512] : register(t0, space0);
#else
Texture2D<float> NoiseTexture : register(t0);
#endif

SamplerState TrilinearSampler : register(s0);

#ifdef _VS

cbuffer viewBuf : register(b0)
{
    float4x4 ViewProjectionMatrix;
};

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

    float2 uvx0 = float2(input.UV.x < 1.0f ? input.UV.x + CellSize : input.UV.x, input.UV.y); 
    float2 uvx1 = float2(input.UV.x > 0.0f ? input.UV.x - CellSize : input.UV.x, input.UV.y);

    float2 uvy0 = float2(input.UV.x, input.UV.y < 1.0f ? input.UV.y + CellSize : input.UV.y);
    float2 uvy1 = float2(input.UV.x, input.UV.y > 0.0f ? input.UV.y - CellSize : input.UV.y);

    float sx = SampleHeight(uvx0) - SampleHeight(uvx1);
    float sy = SampleHeight(uvy0) - SampleHeight(uvy1);

    if(input.UV.x <= 0.0f || input.UV.x >= 1.0f)
        sx *= 2.0f;

    if(input.UV.y <= 0.0f || input.UV.y >= 1.0f)
        sy *= 2.0f;

    float4 WorldPos = float4(input.UV.x * TileScale.x + TileWorldPos.x, height * Height, input.UV.y * TileScale.y + TileWorldPos.y, 1.f);

    output.Position = mul(ViewProjectionMatrix, WorldPos);
    output.UV = input.UV;
    output.Normal = normalize(float3(-sx * TileScale.x, 2.0f * Height, sy * TileScale.y));

    return output;
};

#endif

#ifdef _PS



float4 main(PS_INPUT input) : SV_Target0
{
#if _BINDLESS
    float Noise = t_tex2d_float[NoiseTex].Sample(TrilinearSampler, input.UV).r;
#else
    float Noise = NoiseTexture.Sample(TrilinearSampler, input.UV).r;
#endif
    return float4(saturate(input.Normal), 1.0f);
    return float4(Noise.rrr, 1.0f);
};

#endif