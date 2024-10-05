struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float4 col : COLOR0;
    float2 uv  : TEXCOORD0;
};

#ifdef _VS

cbuffer vertexBuffer : register(b0)
{
    float4x4 ProjectionMatrix;
};

struct VS_INPUT
{
    float2 pos : POSITION;
    float4 col : COLOR0;
    float2 uv  : TEXCOORD0;
};

void main(VS_INPUT input, out PS_INPUT output)
{
    output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));
    output.col = input.col;
    output.uv  = input.uv;
};

#endif

#ifdef _PS

cbuffer pixelBuffer : register(b1)
{
    uint SrvIndex;
    float3 __pad;
};

#if _BINDLESS
Texture2D<float4> t_tex2d[512] : register(t0, space0);
#else
Texture2D texture0;
#endif

SamplerState ImGuiSamp : register(s0);

float4 main(PS_INPUT input) : SV_Target0
{
#if _BINDLESS
    float4 out_col = input.col * t_tex2d[SrvIndex].Sample(ImGuiSamp, input.uv);
#else
    float4 out_col = input.col * texture0.Sample(ImGuiSamp, input.uv);
#endif

    return out_col; 
};

#endif