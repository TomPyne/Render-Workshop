#include "ShaderDefines.h"

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
    float3 __Pad;
};

ConstantBuffer<DynamicUniforms_s> c_Dynamic : register(b0);
ConstantBuffer<ViewUniforms_s> c_View : register(b1);
ConstantBuffer<ModelUniforms_s> c_Model : register(b2);

StructuredBuffer<float3> t_sbuf_f3[8192] : register(t0, space0);

struct Interpolants_s
{
    float4 Position : SV_POSITION;
    float3 WorldPosition : WORLDPOS;
};

#ifdef _VS

void main(in uint VertexID : SV_VertexID, out Interpolants_s Output)
{
    float3 Position = t_sbuf_f3[c_Model.PositionBufferIndex][VertexID];
    float4 WorldPosition = mul(c_Dynamic.ModelMatrix, float4(Position, 1.0f));
    Output.Position = mul(c_View.ViewProjectionMatrix, WorldPosition);
    Output.WorldPosition = WorldPosition.xyz;
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

void main(in Interpolants_s Input, out float4 Output : SV_TARGET)
{
    Output = float4(Checkerboard3D(Input.WorldPosition, float3(1.0f, 0.04f, 0.68f), float3(0.0f, 0.0f, 0.0f), 10.0f), 1.0f);
}

#endif

