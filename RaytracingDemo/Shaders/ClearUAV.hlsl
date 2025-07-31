// Use macros to create permutations of this.

#if defined(F1)
    #define CLEAR_FORMAT float
    #define CLEAR_PAD float3 _pad1;
#elif defined(F2)
    #define CLEAR_FORMAT float2
    #define CLEAR_PAD float2 __pad1;
#else
    #error Unsupported format
#endif

struct Uniforms
{
    uint UAVIndex;
    uint2 Dimensions;
    float __pad0;

    CLEAR_FORMAT ClearVal;
    CLEAR_PAD
};
ConstantBuffer<Uniforms> c_Uniforms : register(b1, space0);

RWTexture2D<CLEAR_FORMAT> u_tex2d[8192] : register(u0, space0);

[NumThreads(8, 8, 1)]
void main(uint3 DispatchThreadId : SV_DispatchThreadID)
{
    if(all(DispatchThreadId.xy < c_Uniforms.Dimensions))
    {
        u_tex2d[c_Uniforms.UAVIndex][DispatchThreadId.xy] = c_Uniforms.ClearVal;
    }
}
