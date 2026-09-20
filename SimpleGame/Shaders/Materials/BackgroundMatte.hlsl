struct MaterialUniforms_s
{    
    float3 Color;
    float Contrast;

    float Brightness;
    uint MatteTextureIndex;
    float2 __Pad;
};

#include "../../../Game/Shaders/MeshMaterial.h"

struct Interpolants_s
{
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
    float2 UV0 : TEXCOORD0;
};

#ifdef _VS

void main(in uint VertexID : SV_VertexID, out Interpolants_s Output)
{
    VS_PosNormalUV0(VertexID, Output.Position, Output.Normal, Output.UV0);
}

#endif // #ifdef _VS

#ifdef _PS

#include "../../../Game/Shaders/Samplers.h"

Texture2D<float4> t_tex2d_f4[8192] : register(t0, space0);

void main(in Interpolants_s Input, out PSOutput_s Output)
{
    float4 Matte = t_tex2d_f4[c_Material.MatteTextureIndex].Sample(SharedClampedSampler, Input.UV0);

    if(Matte.a < 0.333f)
    {
        discard;
    }

    // TODO: Emissive
    float3 Emissive = c_Material.Color * Matte.rgb;
    Emissive = pow(Emissive, c_Material.Contrast);
    Emissive = Emissive * c_Material.Brightness;

    MaterialOutput_Default(Input.Normal, Output);
    MaterialOutput_Specular(0.0f, Output);
    MaterialOutput_Roughness(1.0f, Output);
    MaterialOutput_Emissive(Emissive, Output);
}

#endif

