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
    uint SceneRoughnessMetallicTextureIndex;
    uint DrawMode;
};

#define DRAWMODE_LIT 0
#define DRAWMODE_COLOR 1
#define DRAWMODE_NORMAL 2
#define DRAWMODE_ROUGHNESS 3
#define DRAWMODE_METALLIC 4
#define DRAWMODE_LIGHTING 5

ConstantBuffer<DeferredData> c_Deferred : register(b1);
Texture2D<float4> t_tex2d[8192] : register(t0, space0);
SamplerState ClampedSampler : register(s1);

struct PS_OUTPUT
{
    float4 Color : SV_TARGET0;
};

void main(in PS_INPUT Input, out PS_OUTPUT Output)
{
    float3 Normal = normalize(t_tex2d[c_Deferred.SceneNormalTextureIndex].SampleLevel(ClampedSampler, Input.UV, 0u).rgb);
    float3 Color = t_tex2d[c_Deferred.SceneColorTextureIndex].SampleLevel(ClampedSampler, Input.UV, 0u).rgb;
    float2 RoughnessMetallic = t_tex2d[c_Deferred.SceneRoughnessMetallicTextureIndex].SampleLevel(ClampedSampler, Input.UV, 0u).rg;

    if(c_Deferred.DrawMode != DRAWMODE_LIT)
    {
        if(c_Deferred.DrawMode == DRAWMODE_COLOR)
        {
            Output.Color = float4(Color.rgb, 1.0f);
            return;
        }
        else if(c_Deferred.DrawMode == DRAWMODE_NORMAL)
        {
            Output.Color = float4((Normal + float3(1, 1, 1)) * 0.5f, 1.0f);
            return;
        }
        else if(c_Deferred.DrawMode == DRAWMODE_ROUGHNESS)
        {
            Output.Color = float4(RoughnessMetallic.rrr, 1.0f);
            return;
        }
        else if(c_Deferred.DrawMode == DRAWMODE_METALLIC)
        {
            Output.Color = float4(RoughnessMetallic.ggg, 1.0f);
            return;
        }
        else if(c_Deferred.DrawMode == DRAWMODE_LIGHTING)
        {
            Color = float3(1, 1, 1);
        }
    }

    const float3 LightDir = normalize(float3(0.5, -1, 1));
    float Radiance = saturate(dot(LightDir, -Normal));

    Output.Color = float4((float3(0.1, 0.1, 0.1) * Color.rgb) + (Color.rgb * Radiance), 1.0f);
}

#endif