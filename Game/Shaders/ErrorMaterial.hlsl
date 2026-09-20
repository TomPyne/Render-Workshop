struct MaterialUniforms_s
{
    float4 Dummy;
};

#include "MeshMaterial.h"

struct Interpolants_s
{
    float4 Position : SV_POSITION;
    float3 WorldPosition : WORLDPOS;
    float3 Normal : NORMAL;
};

#ifdef _VS

void main(in uint VertexID : SV_VertexID, out Interpolants_s Output)
{
    float3 Position = t_sbuf_f3[c_Model.PositionBufferIndex][VertexID];
    float4 WorldPosition = mul(c_Dynamic.ModelMatrix, float4(Position, 1.0f));
    
    Output.WorldPosition = ModelToWorld(LoadPosition(VertexID));
    Output.Position = WorldToClip(Output.WorldPosition);
    Output.Normal = NormalModelToWorld(LoadNormal(VertexID));
}

#endif // #ifdef _VS

#ifdef _PS

float3 Checkerboard3D(float3 Pos, float3 ColorA, float3 ColorB, float Scale) {
    // Scale the incoming position
    float3 ScaledPos = floor(Pos * Scale);
        
    // Sum the X, Y, and Z components
    float Sum = ScaledPos.x + ScaledPos.y + ScaledPos.z;
    
    // Use fmod to alternate between 0 and 1
    float Check = fmod(Sum, 2.0);
    // Handle potential negative values if needed
    Check = abs(Check); 
    
    // Lerp between the two float3 colors
    return lerp(ColorA, ColorB, Check);
}

void main(in Interpolants_s Input, out PSOutput_s Output)
{
    Output.AlbedoMetallic = float4(Checkerboard3D(Input.WorldPosition, float3(1.0f, 0.04f, 0.68f), float3(0.0f, 0.0f, 0.0f), 10.0f), 0.0f);
    Output.NormalRoughness = float4(Input.Normal, 1.0f);
}

#endif

