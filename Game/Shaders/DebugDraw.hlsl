#include "View.h"
#include "DrawConstants.h"

// Must match DebugVertex_s in DebugDraw.h
struct DebugVertex_s
{
    float3 Pos;
    uint Color;
};

ConstantBuffer<ViewUniforms_s> c_View : register(b1);

struct Interpolants_s
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR0;
};

#ifdef _VS

void main(in uint VertexID : SV_VertexID, out Interpolants_s Output)
{
    const DebugVertex_s Vertex = t_DrawConstants.Load<DebugVertex_s>(VertexID * 16);

    Output.Position = mul(c_View.ViewProjectionMatrix, float4(Vertex.Pos, 1.0f));
    Output.Color = float4(Vertex.Color & 0xFF, (Vertex.Color >> 8) & 0xFF, (Vertex.Color >> 16) & 0xFF, Vertex.Color >> 24) / 255.0f;
}

#endif // #ifdef _VS

#ifdef _PS

void main(in Interpolants_s Input, out float4 Output : SV_TARGET)
{
    Output = Input.Color;
}

#endif
