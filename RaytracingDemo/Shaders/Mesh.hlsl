struct ViewData
{
    float4x4 ViewProjectionMatrix;
    float3 CameraPos;
    float __pad;
};

ConstantBuffer<ViewData> c_View : register(b0);

struct PS_INPUT
{
    float4 SVPosition : SV_POSITION;
    float2 UV : TEXCOORD0;
    float3 Normal : NORMAL;
};

#ifdef _VS

struct VS_INPUT
{
    float3 Position : POSITION;
    float2 Texcoord : TEXCOORD;
    float3 Normal : NORMAL;
};

void main(in VS_INPUT Input, out PS_INPUT Output)
{
    Output.SVPosition = mul(c_View.ViewProjectionMatrix, float4(Input.Position, 1.0f));
    Output.Normal = Input.Normal;
    Output.UV = Input.Texcoord;
}

#endif // _VS

#ifdef _PS

struct MaterialData
{
    float3 Albedo;
    uint AlbedoTextureIndex;
};

struct PS_OUTPUT
{
    float4 Color : SV_TARGET0;
    float4 Normal : SV_TARGET1;
};

ConstantBuffer<MaterialData> c_Material : register(b1);
Texture2D<float4> t_Tex2d[1024] : register(t0, space0);
SamplerState s_ClampedSampler : register(s1);

void main(in PS_INPUT Input, out PS_OUTPUT Output)
{
    float3 Albedo = c_Material.Albedo;
    if(c_Material.AlbedoTextureIndex != 0)
    {
        Albedo *= t_Tex2d[c_Material.AlbedoTextureIndex].Sample(s_ClampedSampler, Input.UV).rgb;
    }
    Output.Color = float4(Albedo, 1.0f);
    Output.Normal = float4(Input.Normal, 0.0f);
}

#endif