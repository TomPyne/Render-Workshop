struct MaterialUniforms_s
{
    float3 Albedo;
    float __Pad;
};

#include "../MeshMaterial.h"

struct Interpolants_s
{
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
};

#ifdef _VS

void main(in uint VertexID : SV_VertexID, out Interpolants_s Output)
{
    VS_PosNormal(VertexID, Output.Position, Output.Normal);
}

#endif // #ifdef _VS

#ifdef _PS

void main(in Interpolants_s Input, out PSOutput_s Output)
{
    MaterialOutput_Default(Input.Normal, Output);
    MaterialOutput_Albedo(c_Material.Albedo, Output);
}

#endif

