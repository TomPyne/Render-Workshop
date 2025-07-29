// Use macros to create permutations of this.

struct Uniforms
{
    uint UAVIndex;
    uint2 Dimensions;
    float ClearVal;
};
ConstantBuffer<Uniforms> c_Uniforms : register(b1, space0);

RWTexture2D<float> u_tex2d_float[8192] : register(u0, space0);

[NumThreads(8, 8, 1)]
void main(uint3 DispatchThreadId : SV_DispatchThreadID)
{
    if(all(DispatchThreadId.xy < c_Uniforms.Dimensions))
    {
        u_tex2d_float[c_Uniforms.UAVIndex][DispatchThreadId.xy] = c_Uniforms.ClearVal;
    }
}
