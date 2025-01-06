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

struct DeferredData
{
    uint SceneColorTextureIndex;
    uint SceneNormalTextureIndex;
    float2 __pad;
};

ConstantBuffer<DeferredData> c_Deferred : register(b1);
Texture2D<float4> t_tex2d[1024] : register(t0, space0);
SamplerState ClampedSampler : register(s1);

struct PS_OUTPUT
{
    float4 Color : SV_TARGET0;
};

void main(in PS_INPUT Input, out PS_OUTPUT Output)
{
    const float3 LightDir = normalize(float3(0.5, -1, 1));

    float3 Normal = normalize(t_tex2d[c_Deferred.SceneNormalTextureIndex].SampleLevel(ClampedSampler, Input.UV, 0u).rgb);

    float Radiance = saturate(dot(LightDir, -Normal));

    float4 Color = t_tex2d[c_Deferred.SceneColorTextureIndex].SampleLevel(ClampedSampler, Input.UV, 0u).rgba;

    Output.Color = float4((float3(0.1, 0.1, 0.1) * Color.rgb) + (Color.rgb * Radiance), 1.0f);
}

#endif