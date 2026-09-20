#ifdef _PS

#include "Samplers.h"
#include "ScreenPassShared.h"

struct DeferredData_s
{
    uint SceneColorMetallicTextureIndex;
    uint SceneNormalRoughnessTextureIndex;
    uint SceneEmissiveSpecularTextureIndex;
    float __Pad;
};

ConstantBuffer<DeferredData_s> c_Deferred : register(b2);
Texture2D<float4> t_tex2d_f4[8192] : register(t0, space0);


void main(in Interpolants_s Input, out float4 Output : SV_TARGET)
{
    float4 ColorMetallic = t_tex2d_f4[c_Deferred.SceneColorMetallicTextureIndex].SampleLevel(SharedClampedSampler, Input.UV, 0u);
    float4 NormalRoughness = t_tex2d_f4[c_Deferred.SceneNormalRoughnessTextureIndex].SampleLevel(SharedClampedSampler, Input.UV, 0u);
    float4 SpecularEmissive = t_tex2d_f4[c_Deferred.SceneEmissiveSpecularTextureIndex].SampleLevel(SharedClampedSampler, Input.UV, 0u);
    float3 L = normalize(float3(-0.5, 1.0f, 0.5f));
    float NoL = dot(normalize(NormalRoughness.xyz), L);
    float3 DummyLit = ColorMetallic.rgb * saturate(NoL + 0.1f);

    Output = float4(DummyLit + SpecularEmissive.rgb, 0.0f);
}

#endif // #ifdef _PS