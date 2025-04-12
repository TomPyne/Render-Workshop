struct ViewData_s
{
    float4x4 ViewProjectionMatrix;
    float3 CameraPos;
    uint DebugMeshID;
};

ConstantBuffer<ViewData_s> c_View : register(b1);

struct PixelInputs_s
{
    float4 SVPosition : SV_POSITION;
    float2 UV : TEXCOORD0;
    float3 Normal : NORMAL0;
    uint MeshletIndex : COLOR0;
};

// Shared binds for vertex processing stages
#if defined(_MS) || defined(_VS)

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

ConstantBuffer<MeshData_s> c_Mesh : register(b2);

StructuredBuffer<float3> t_sbuf_f3[8192] : register(t0, space0);
StructuredBuffer<float4> t_sbuf_f4[8192] : register(t0, space1);
StructuredBuffer<float2> t_sbuf_f2[8192] : register(t0, space2);
StructuredBuffer<uint> t_sbuf_uint[8192] : register(t0, space3);

PixelInputs_s GetVertexAttributes(uint MeshletIndex, uint VertexIndex)
{
    float3 Position = t_sbuf_f3[c_Mesh.PositionBufSRVIndex][VertexIndex];
    float3 Normal = c_Mesh.NormalBufSRVIndex != 0 ? t_sbuf_f3[c_Mesh.NormalBufSRVIndex][VertexIndex] : float3(0, 0, 0);
    float2 UV = c_Mesh.TexcoordBufSRVIndex != 0 ? t_sbuf_f2[c_Mesh.TexcoordBufSRVIndex][VertexIndex] : float2(0, 0);

    PixelInputs_s Out;
    Out.SVPosition =  mul(c_View.ViewProjectionMatrix, float4(Position, 1.0f));
    Out.Normal = Normal;
    Out.UV = float2(UV.x, 1.0f - UV.y);
    Out.MeshletIndex = MeshletIndex;

    return Out;
}

#endif

#ifdef _VS

struct DrawConstants_s
{
    uint IndexOffset;
};

ConstantBuffer<DrawConstants_s> c_Draw : register(b0);

void main(in uint VertexID : SV_VertexID, out PixelInputs_s Output)
{
    uint VertexIndex = t_sbuf_uint[c_Mesh.IndexBufSRVIndex][c_Draw.IndexOffset + VertexID];

    Output = GetVertexAttributes(c_Draw.IndexOffset, VertexIndex);
}

#endif // _VS

#ifdef _MS

struct Meshlet_s
{
    uint VertCount;
    uint VertOffset;
    uint PrimCount;
    uint PrimOffset;
};

struct DrawConstants_s
{
    uint MeshletOffset;
};

ConstantBuffer<DrawConstants_s> c_Draw : register(b0);

StructuredBuffer<Meshlet_s> t_sbuf_meshlet[8192] : register(t0, space4);

uint3 UnpackPrimitive(uint Primitive)
{
    // Unpacks a 10 bits per index triangle from a 32-bit uint.
    return uint3(Primitive & 0x3FF, (Primitive >> 10) & 0x3FF, (Primitive >> 20) & 0x3FF);
}

uint3 GetPrimitive(Meshlet_s Meshlet, uint Index)
{
    return UnpackPrimitive(t_sbuf_uint[c_Mesh.PrimitiveIndexBufSRVIndex][Meshlet.PrimOffset + Index]);
}

uint GetVertexIndex(Meshlet_s Meshlet, uint LocalIndex)
{
    return t_sbuf_uint[c_Mesh.UniqueVertexIndexBufSRVIndex][LocalIndex + Meshlet.VertOffset];
}

[NumThreads(128, 1, 1)]
[OutputTopology("triangle")]
void main(
    in uint GroupThreadID : SV_GroupThreadID,
    in uint GroupID : SV_GroupID,
    out indices uint3 Triangles[128],
    out vertices PixelInputs_s Verts[128])
{
    Meshlet_s Meshlet = t_sbuf_meshlet[c_Mesh.MeshletBufSRVIndex][c_Draw.MeshletOffset + GroupID];

    SetMeshOutputCounts(Meshlet.VertCount, Meshlet.PrimCount);

    if(GroupThreadID < Meshlet.PrimCount)
    {
        Triangles[GroupThreadID] = GetPrimitive(Meshlet, GroupThreadID);
    }

    if(GroupThreadID < Meshlet.VertCount)
    {
        uint VertexIndex = GetVertexIndex(Meshlet, GroupThreadID);
        Verts[GroupThreadID] = GetVertexAttributes(GroupID, VertexIndex);
    }
}

#endif

#ifdef _PS

struct MaterialData_s
{
    float3 Albedo;
    uint AlbedoTextureIndex;

    uint NormalTextureIndex;
    float Roughness;
    float Metallic;
    uint RoughnessMetallicTextureIndex;
};

struct PixelOutputs_s
{
    float4 Color : SV_TARGET0;
    float4 Normal : SV_TARGET1;
    float2 RoughnessMetallic : SV_TARGET2;
};

ConstantBuffer<MaterialData_s> c_Material : register(b3);
Texture2D<float4> t_Tex2d[8192] : register(t0, space0);
SamplerState s_ClampedSampler : register(s1);

void main(in PixelInputs_s Input, out PixelOutputs_s Output)
{            
    float3 Albedo = c_Material.Albedo;
    if(c_Material.AlbedoTextureIndex != 0)
    {
        Albedo *= t_Tex2d[c_Material.AlbedoTextureIndex].Sample(s_ClampedSampler, Input.UV).rgb;
    }

    float3 Normal = Input.Normal;
    // TODO: Texture normals

    float Roughness = c_Material.Roughness;
    float Metallic = c_Material.Metallic;

    if(c_Material.RoughnessMetallicTextureIndex != 0)
    {
        float2 RoughnessMetallicSample = t_Tex2d[c_Material.RoughnessMetallicTextureIndex].Sample(s_ClampedSampler, Input.UV).rg;
    }
    
    if(c_View.DebugMeshID != 0)
    {
        float3 MeshletColor = float3(
            float(Input.MeshletIndex & 1),
            float(Input.MeshletIndex & 3) / 4,
            float(Input.MeshletIndex & 7) / 8);

        Albedo *= MeshletColor;
    }

    Output.Color = float4(Albedo, 1.0f);
    Output.Normal = float4(Input.Normal, 0.0f);
    Output.RoughnessMetallic = float2(Roughness, Metallic);
}

#endif