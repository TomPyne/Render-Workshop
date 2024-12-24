struct PS_INPUT
{
    float4 Position : SV_POSITION;
#if VERT_NORMAL
    float3 Normal : NORMAL;
#endif // VERT_NORMAL
};

cbuffer meshBuf : register(b1)
{
    row_major float4x4 MeshTransformMatrix;
    float4 Color;
}

#ifdef _VS

cbuffer viewBuf : register(b0)
{
    float4x4 ViewProjectionMatrix;
};

struct VS_INPUT
{
    float3 Position : POSITION;
#if VERT_NORMAL
    float3 Normal : NORMAL;
#endif // VERT_NORMAL
};

void main(in VS_INPUT Input, out PS_INPUT Output)
{
    float4 WorldPos = mul(MeshTransformMatrix, float4(Input.Position;, 1.f));
    
    Output.Position = mul(ViewProjectionMatrix, WorldPos);

#if VERT_NORMAL
    Output.Normal = Input.Normal;
#endif // VERT_NORMAL
};

#endif

#ifdef _PS

float4 main(PS_INPUT input) : SV_Target0
{
    return float4(Color, 1.0f);
};

#endif