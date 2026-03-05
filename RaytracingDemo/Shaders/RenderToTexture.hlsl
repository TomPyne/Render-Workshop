struct PS_INPUT
{
    float4 SVPosition : SV_POSITION;
    float2 UV : TEXCOORD0;
};

#ifdef _VS

static const float2 Verts[6] = 
{
    float2(0, 1), float2(1, 1), float2(1, 0),
    float2(1, 0), float2(0, 0), float2(0, 1)
};

void main(uint VertexID : SV_VertexID, out PS_INPUT Output)
{
    const float2 Vert = Verts[VertexID];

    Output.SVPosition = float4(Vert * 2 - 1, 0.5f, 1.0f);
    Output.UV = Vert;
    Output.UV.y = 1.0f - Output.UV.y;
}

#endif

#ifdef _PS

struct Uniforms
{
    uint32_t InputTexture;
    float3 __Pad0;
};

ConstantBuffer<Uniforms> c_G : register(b1);
Texture2D<float4> t_tex2d_f4[8192] : register(t0, space0);
SamplerState ClampedSampler : register(s1);

float4 main(in PS_INPUT Input) : SV_TARGET0
{
    float4 InputColor = t_tex2d_f4[c_G.InputTexture].SampleLevel(ClampedSampler, Input.UV, 0u);
    return float4(InputColor.rgb, 1.0f);
}

#endif