#include "ShaderDefines.h"

struct ViewUniforms_s
{
    float4x4 ViewProjectionMatrix;
};

struct ModelUniforms_s
{
    float4x4 ModelMatrix;
    uint PositionBufferIndex;
    float3 __Pad;
};

struct MaterialUniforms_s
{
    float3 Color;
    float __Pad;
};

ConstantBuffer<ViewUniforms_s> c_View : register(b1);
ConstantBuffer<ModelUniforms_s> c_Model : register(b2);
ConstantBuffer<MaterialUniforms_s> c_Material : register(b3);

StructuredBuffer<float3> t_sbuf_f3 : register(t0, space0);

struct Interpolants_s
{
    float4 Position : SV_POSITION;
};

#ifdef _VS

void main(in uint VertexID : SV_VertexID, out Interpolants_s Output)
{
    float3 Position = t_sbuf_f3[c_Model.PositionBufferIndex][VertexID];
    float4 WorldPosition = mul(c_Model.ModelMatrix, float4(Position, 1.0f));
    Output.Position = mul(c_View.ViewProjectionMatrix, WorldPosition);
}

#endif // #ifdef _VS

#ifdef _PS

void main(in Interpolants_s Input, out float4 Output : SV_TARGET)
{
    Output = float4(c_Material.Color, 1.0f);
}

#endif

