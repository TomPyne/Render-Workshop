struct Uniforms
{
    float4x4 ViewProjectionMatrix;
    float3 CameraPos;
    float __Pad;
};

ConstantBuffer<Uniforms> c_G : register(b1);

struct PixelInputs
{
    float4 SVPosition : SV_Position;
    float3 LocalPos : POSITION;
};

#if _VS

struct VertexInputs
{
    float3 Position : POSITION;
};

void main(in VertexInputs Input, out PixelInputs Output)
{   
    Output.LocalPos = Input.Position + c_G.CameraPos;
    Output.SVPosition = mul(c_G.ViewProjectionMatrix, float4(Output.LocalPos, 1.0f));
}

#endif

#if _PS
float4 main(in PixelInputs Input) : SV_TARGET
{   
    float Height = saturate(normalize(Input.LocalPos.y));
    return float4(lerp(float3(1, 1, 1), float3(0, 0, 1), Height), 1.0f);
}
#endif




