#ifndef CBV_SLOT
#error Must define CBV_SLOT
#endif

struct Uniforms
{
    float4x4 ViewProjectionMatrix;
    float3 CameraPos;
    float __Pad;
};

ConstantBuffer<Uniforms> c_G : register(CBV_SLOT);

struct PixelInputs
{
    float4 SVPosition : SV_Position;
    float3 Direction : DIRECTION;
};

#if _VS

struct VertexInputs
{
    float3 Position : POSITION;
};

void main(in VertexInputs Input, out PixelInputs Output)
{   
    float3 LocalPos = Input.Position + c_G.CameraPos;
    Output.Direction = normalize(Input.Position);
    Output.SVPosition = mul(c_G.ViewProjectionMatrix, float4(LocalPos, 1.0f));
}

#endif

#if _PS
float4 main(in PixelInputs Input) : SV_TARGET
{   
    float Height = saturate(normalize(Input.Direction).y);
    return float4(lerp(float3(0.7, 0.7, 0.7), float3(0.2, 0.2, 0.8), Height), 1.0f);
}
#endif




