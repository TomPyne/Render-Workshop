struct MaterialUniforms_s
{
    float3 Color;
    float __Pad;
};

#include "MeshMaterial.h"

struct Interpolants_s
{
    float4 Position : SV_POSITION;
    float4 PrevPosition : PREV_POSITION;
    float3 Normal : NORMAL;
};

#ifdef _VS

void main(in uint VertexID : SV_VertexID, out Interpolants_s Output)
{
    Output.Position = ModelToClip(LoadPosition(VertexID));
    Output.PrevPosition = ModelToPrevClip(LoadPosition(VertexID));
    Output.Normal = NormalModelToWorld(LoadNormal(VertexID));
}

#endif // #ifdef _VS

#ifdef _PS

void main(in Interpolants_s Input, out PSOutput_s Output)
{
    Output.AlbedoMetallic = float4(c_Material.Color, 0.0f);
    Output.NormalRoughness = float4(Input.Normal, 1.0f);

    MaterialOutput_Default(normalize(Input.Normal), Output);
    MaterialOutput_Albedo(c_Material.Color, Output);
    MaterialOutput_Metallic(0.0f, Output);
    MaterialOutput_Roughness(0.8f, Output);
    MaterialOutput_Velocity(Input.Position, Input.PrevPosition, Output);
}

#endif

