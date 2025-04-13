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
    float4x4 InverseViewProjection;
    uint SceneColorTextureIndex;
    uint SceneNormalTextureIndex;
    uint SceneRoughnessMetallicTextureIndex;
    uint DepthTextureIndex;
    uint DrawMode;
    float3 CamPosition;
};

#define DRAWMODE_LIT 0
#define DRAWMODE_COLOR 1
#define DRAWMODE_NORMAL 2
#define DRAWMODE_ROUGHNESS 3
#define DRAWMODE_METALLIC 4
#define DRAWMODE_DEPTH 5
#define DRAWMODE_POSITION 6
#define DRAWMODE_LIGHTING 7

ConstantBuffer<DeferredData> c_Deferred : register(b1);
Texture2D<float4> t_tex2d_f4[8192] : register(t0, space0);
Texture2D<float2> t_tex2d_f2[8192] : register(t0, space1);
Texture2D<float> t_tex2d_f1[8192] : register(t0, space2);
SamplerState ClampedSampler : register(s1);

#define PI 3.141592653589793f

float Lambert()
{
    return 1.0f / PI;
}

float GGX(float NoH, float A)
{
    float A2 = A * A;
    float F = (NoH * A2 - NoH) * NoH + 1.0f;
    return A2 / (PI * F * F);
}

float SmithGGXCorrelated(float NoV, float NoL, float A)
{
    float A2 = A * A;
    float GGXL = NoV * sqrt((-NoL * A2 + NoL) * NoL + A2);
    float GGXV = NoL * sqrt((-NoV * A2 + NoV) * NoV + A2);
    return 0.5f / (GGXV + GGXL);
}

float3 Schlick(float U, float3 F0)
{
    static const float3 F90 = float3(1.0f, 1.0f, 1.0f);
    return F0 + (F90 - F0) * pow(1.0f - U, 5.0f);
}

struct PS_OUTPUT
{
    float4 Color : SV_TARGET0;
};

float3 ViewPositionFromDepth(float2 UV, float Depth)
{
    float x = UV.x * 2 - 1;
    float y = (1 - UV.y) * 2 - 1;
    float4 ProjectedPos = float4(x, y, Depth, 1.0f);

    ProjectedPos = mul(c_Deferred.InverseViewProjection, ProjectedPos);

    return ProjectedPos.xyz / ProjectedPos.w;
}

void main(in PS_INPUT Input, out PS_OUTPUT Output)
{
    float3 Normal = normalize(t_tex2d_f4[c_Deferred.SceneNormalTextureIndex].SampleLevel(ClampedSampler, Input.UV, 0u).rgb);
    float3 Color = t_tex2d_f4[c_Deferred.SceneColorTextureIndex].SampleLevel(ClampedSampler, Input.UV, 0u).rgb;
    float2 RoughnessMetallic = t_tex2d_f2[c_Deferred.SceneRoughnessMetallicTextureIndex].SampleLevel(ClampedSampler, Input.UV, 0u).rg;
    RoughnessMetallic = float2(0.1f, 1.0f);
    float Depth = t_tex2d_f1[c_Deferred.DepthTextureIndex].SampleLevel(ClampedSampler, Input.UV, 0u).r;
    float3 Position = ViewPositionFromDepth(Input.UV, Depth);

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
        else if(c_Deferred.DrawMode == DRAWMODE_DEPTH)
        {
            Output.Color = float4(Depth.rrr, 1.0f);
            return;
        }
        else if(c_Deferred.DrawMode == DRAWMODE_POSITION)
        {
            Output.Color = float4(frac(abs(Position)), 1.0f);
            return;
        }
        else if(c_Deferred.DrawMode == DRAWMODE_LIGHTING)
        {
            Color = float3(1, 1, 1);
        }
    }

    float3 L = normalize(-float3(1.0, -1, 1.0));
    float3 V = normalize(Position - c_Deferred.CamPosition);
    float3 H = normalize(V + L);

    float NoV = abs(dot(Normal, V)) + 0.00001f;
    float NoL = saturate(dot(Normal, L));
    float NoH = saturate(dot(Normal, H));
    float LoH = saturate(dot(L, H));
    
    float3 Reflectance = 0.55f;
    float3 F0 = 0.16f * Reflectance * Reflectance * (1.0f - RoughnessMetallic.g) + Color * RoughnessMetallic.g;

    float D = GGX(NoH, RoughnessMetallic.r);
    float3 F = Schlick(LoH, F0);
    float Vis = SmithGGXCorrelated(NoV, NoL, RoughnessMetallic.r);

    float3 Diffuse = (1.0f - RoughnessMetallic.g) * Color;

    float3 SpecularTerm = (D * Vis) * F; // TODO energy loss compensation
    float3 DiffuseTerm = Diffuse * Lambert();


    Output.Color = float4((SpecularTerm + DiffuseTerm) * NoL, 1.0f);
    //Output.Color = float4(Depth < 1.0f ? F : float3(0, 0, 0), 1.0f);
    Output.Color = float4(NoL.rrr, 1.0f);
}

#endif