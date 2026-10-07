#include "../ShaderDefines.h"
#include "../View.h"
#include "ParticleCommon.h"

struct MaterialUnforms_s
{
    float3 Color;
    uint TextureIndex;
};

ConstantBuffer<ViewUniforms_s> c_View : register(b1);
ConstantBuffer<MaterialUnforms_s> c_Material : register(b3);

struct Interpolants_s
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

#ifdef _VS

StructuredBuffer<Particle_s> t_sbuf_particle[8192] : register(t1, space1);
StructuredBuffer<uint> t_sbuf_u1[8192] : register(t1, space2);

static const float2 Verts[6] =
{
    float2(0, 1), float2(1, 1), float2(1, 0),
    float2(1, 0), float2(0, 0), float2(0, 1)
};

void main(in uint VertexID : SV_VertexID, in uint InstanceID : SV_InstanceID, out Interpolants_s Output)
{
    const float2 Vert = Verts[VertexID];

    const uint ParticleIndex = t_sbuf_u1[c_Particles.AliveListSRV][InstanceID];
    const float3 Position = t_sbuf_particle[c_Particles.ParticlesSRV][ParticleIndex].Position;

    const float2 Corner = (Vert - 0.5f) * c_Particles.Scale;
    const float3 World = Position + c_View.CamRight * Corner.x + c_View.CamUp * Corner.y;

    Output.Position = mul(c_View.ViewProjectionMatrix, float4(World, 1.0f));
    Output.TexCoord = float2(Vert.x, 1.0f - Vert.y);
}

#endif // #ifdef _VS

#ifdef _PS

#include "../Samplers.h"

Texture2D<float4> t_tex2d_f4[8192] : register(t1, space0);

void main(in Interpolants_s Input, out float4 Output : SV_TARGET)
{
    float4 Color = t_tex2d_f4[c_Material.TextureIndex].Sample(SharedWrappedSampler, Input.TexCoord);
    Color.rgb *= c_Material.Color * Color.a; // Additive blend ignores alpha

    Output = Color;
}

#endif
