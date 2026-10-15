struct Uniforms_s
{
    uint SourceTextureIndex;
    uint OutputTextureIndex;
    uint Size;
    uint __Pad;
};

ConstantBuffer<Uniforms_s> c_G : register(b0);

RWTexture2DArray<float4> u_tex2darr_f4[8192] : register(u0, space0);

// Face local 2x2 box, the chain is only a background and a filtered importance sampling source
[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (any(DTid.xy >= c_G.Size))
    {
        return;
    }

    RWTexture2DArray<float4> Source = u_tex2darr_f4[c_G.SourceTextureIndex];

    const uint3 Base = uint3(DTid.xy * 2u, DTid.z);
    const float4 Sum = Source[Base] + Source[Base + uint3(1, 0, 0)] + Source[Base + uint3(0, 1, 0)] + Source[Base + uint3(1, 1, 0)];

    u_tex2darr_f4[c_G.OutputTextureIndex][DTid] = Sum * 0.25f;
}
