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
    float4x4 InverseProjection;
    float4x4 InverseView;
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

float GGX(float NoH, float Roughness)
{
    float A2 = Roughness * Roughness;
    float F = (NoH * A2 - NoH) * NoH + 1.0f;
    return A2 / (PI * F * F);
}

float GeometrySchlickGGX(float NoV, float K)
{
    float Denom = NoV * (1.0 - K) + K;
	
    return NoV / Denom;
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
    float4 ProjectedPos = float4(x, y, Depth,  1.0f);

    ProjectedPos = mul(c_Deferred.InverseProjection, ProjectedPos);

    ProjectedPos /= ProjectedPos.w;

    return mul(c_Deferred.InverseView, ProjectedPos).xyz;
}

void main(in PS_INPUT Input, out PS_OUTPUT Output)
{
    float Depth = t_tex2d_f1[c_Deferred.DepthTextureIndex].SampleLevel(ClampedSampler, Input.UV, 0u).r;
    if(Depth >= 1.0f)
    {
        Output.Color = float4(0, 0, 0, 1);
        return;
    }

    float3 Normal = (t_tex2d_f4[c_Deferred.SceneNormalTextureIndex].SampleLevel(ClampedSampler, Input.UV, 0u).rgb);
    float3 Color = t_tex2d_f4[c_Deferred.SceneColorTextureIndex].SampleLevel(ClampedSampler, Input.UV, 0u).rgb;
    float2 RoughnessMetallic = t_tex2d_f2[c_Deferred.SceneRoughnessMetallicTextureIndex].SampleLevel(ClampedSampler, Input.UV, 0u).rg * float2(0.9, 1.0);
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
            Output.Color = float4((Normal + float3(1, 1, 1)) * 0.5f, 1);
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

    float Roughness = RoughnessMetallic.r * RoughnessMetallic.r;
    float Metallic = RoughnessMetallic.g;

    float3 L = normalize(float3(0.5f, 1, 0.3));
    float3 V = normalize(c_Deferred.CamPosition - Position);
    float3 H = normalize(V + L);

    float NoV = abs(dot(Normal, V)) + 0.00001f;
    float NoL = saturate(dot(Normal, L));
    float NoH = saturate(dot(Normal, H));
    float LoH = saturate(dot(L, H));
    
    float3 Reflectance = 0.55f;
    float3 F0 = 0.16f * Reflectance * Reflectance * (1.0f - Metallic) + Color * Metallic;

    float D = GGX(NoH, Roughness);
    float3 F = Schlick(LoH, F0);
    float Vis = SmithGGXCorrelated(NoV, NoL, Roughness);   

    float3 SpecularTerm = (D * Vis) * F; // TODO energy loss compensation

    float3 Diffuse = (1.0f - Metallic) * Color;
    float3 DiffuseTerm = Diffuse * Lambert();

    float3 Lighting = (SpecularTerm + DiffuseTerm) * NoL * 5.0f;
    Lighting += 0.3f * Diffuse;
    
    Output.Color = float4(Lighting, 1.0f);
}
#endif