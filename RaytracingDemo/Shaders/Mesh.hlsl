struct ViewData_s
{
    float4x4 ViewProjectionMatrix;
    float3 CameraPos;
    float __pad;
};

struct MeshData_s
{
    uint PositionBufSRVIndex;
	uint NormalBufSRVIndex;
	uint TangentBufSRVIndex;
	uint BitangentBufSRVIndex;
	uint TexcoordBufSRVIndex;
    uint IndexBufSRVIndex;
    uint MeshletBufSRVIndex;
    uint UniqueVertexIndexBufSRVIndex;
    uint PrimitiveIndexBufSRVIndex;
    float3 __pad;
};

struct DrawConstants_s
{
    uint IndexOffset;
};

ConstantBuffer<DrawConstants_s> c_Draw : register(b0);
ConstantBuffer<ViewData_s> c_View : register(b1);
ConstantBuffer<MeshData_s> c_Mesh : register(b2);

struct PS_INPUT
{
    float4 SVPosition : SV_POSITION;
    float2 UV : TEXCOORD0;
    float3 Normal : NORMAL;
};

#ifdef _VS

StructuredBuffer<float3> t_sbuf_f3[1024] : register(t0, space0);
StructuredBuffer<float4> t_sbuf_f4[1024] : register(t0, space1);
StructuredBuffer<float2> t_sbuf_f2[1024] : register(t0, space2);
StructuredBuffer<uint> t_sbuf_uint[1024] : register(t0, space3);

void main(in uint VertexID : SV_VertexID, out PS_INPUT Output)
{
    uint Index = t_sbuf_uint[c_Mesh.IndexBufSRVIndex][c_Draw.IndexOffset + VertexID];
    float3 Position = t_sbuf_f3[c_Mesh.PositionBufSRVIndex][Index];

    float3 Normal = c_Mesh.NormalBufSRVIndex != 0 ? t_sbuf_f3[c_Mesh.NormalBufSRVIndex][Index] : float3(0, 0, 0);
    float2 UV = c_Mesh.TexcoordBufSRVIndex != 0 ? t_sbuf_f2[c_Mesh.TexcoordBufSRVIndex][Index] : float2(0, 0);

    Output.SVPosition = mul(c_View.ViewProjectionMatrix, float4(Position, 1.0f));
    Output.Normal = Normal;
    Output.UV = float2(UV.x, 1.0f - UV.y);
}

#endif // _VS

#ifdef _MS

[NumThreads(128, 1, 1)]
[OutputTopology("triangle")]
void main(out PS_INPUT Output)

#endif

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

ConstantBuffer<MaterialData> c_Material : register(b3);
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