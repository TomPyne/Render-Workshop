struct MaterialUniforms_s
{   
    uint SkyTexture;
    float3 __Pad;
};

#include "../../../Game/Shaders/MeshMaterial.h"

struct Interpolants_s
{
    float4 Position : SV_POSITION;
    float4 PrevPosition : PREV_POSITION;
    float3 Normal : NORMAL;
    float2 UV0 : TEXCOORD0;
};

#include "SunTempleShared.h"

#ifdef _VS

void main(in uint VertexID : SV_VertexID, out Interpolants_s Output)
{
    VS_PosNormalUV0(VertexID, Output.Position, Output.PrevPosition, Output.Normal, Output.UV0);
    Output.UV0 *= float2(1.0f, 0.5f);
    Output.Position = Output.Position.xyww;
}

#endif // #ifdef _VS

#ifdef _PS

#include "../../../Game/Shaders/Samplers.h"

Texture2D<float4> t_tex2d_f4[8192] : register(t1, space0);

void main(in Interpolants_s Input, out PSOutput_s Output)
{
    float3 Color = t_tex2d_f4[c_Material.SkyTexture].Sample(SharedWrappedSampler, Input.UV0).rgb * 3.0f;

    MaterialOutput_Default(normalize(Input.Normal), Output);
    MaterialOutput_Emissive(Color, Output);

    MaterialOutput_Velocity(Input.Position, Input.PrevPosition, Output);
}

#endif

