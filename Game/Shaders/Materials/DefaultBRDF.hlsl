#include "../ShaderDefines.h"

struct ViewUniforms_s
{
    float4x4 ViewProjectionMatrix;
};

struct DynamicUniforms_s
{
    float4x4 ModelMatrix;
    float4x4 NormalMatrix;
    float DeterminantSign;
    float3 __Pad;
};

struct ModelUniforms_s
{
    uint PositionBufferIndex;
    uint NormalBufferIndex;
    uint TangentBufferIndex;
    uint Texcoord0BufferIndex;
};

struct MaterialUniforms_s
{
    float3 Albedo;
    float __Pad;
};

ConstantBuffer<DynamicUniforms_s> c_Dynamic : register(b0);
ConstantBuffer<ViewUniforms_s> c_View : register(b1);
ConstantBuffer<ModelUniforms_s> c_Model : register(b2);
ConstantBuffer<MaterialUniforms_s> c_Material : register(b3);

StructuredBuffer<float2> t_sbuf_f2[8192] : register(t0, space0);
StructuredBuffer<float3> t_sbuf_f3[8192] : register(t0, space1);
StructuredBuffer<float4> t_sbuf_f4[8192] : register(t0, space2);

struct Interpolants_s
{
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
    float4 Tangent : TANGENT;
    float2 UV : TEXCOORD0;
};

#ifdef _VS

void main(in uint VertexID : SV_VertexID, out Interpolants_s Output)
{
    float3 Position = t_sbuf_f3[c_Model.PositionBufferIndex][VertexID];
    float3 Normal = t_sbuf_f3[c_Model.NormalBufferIndex][VertexID];
    float4 Tangent = t_sbuf_f4[c_Model.TangentBufferIndex][VertexID];
    float2 UV = t_sbuf_f2[c_Model.Texcoord0BufferIndex][VertexID];

    float4 WorldPosition = mul(c_Dynamic.ModelMatrix, float4(Position, 1.0f));

    Output.Position = mul(c_View.ViewProjectionMatrix, WorldPosition);

    Output.Normal = normalize(mul((float3x3)c_Dynamic.NormalMatrix, Normal));
    Output.Tangent.xyz = normalize(mul((float3x3)c_Dynamic.ModelMatrix, Tangent.xyz));
    Output.Tangent.w = Tangent.w * c_Dynamic.DeterminantSign;

    Output.UV = UV;
}

#endif // #ifdef _VS

#ifdef _PS

void main(in Interpolants_s Input, out float4 Output : SV_TARGET)
{
    float3 Normal = normalize(Input.Normal);

    float Lit = dot(float3(0, 0.707f, -0.707f), Normal);

    Output = float4(saturate(Lit).rrr, 1.0f);
}

#endif

