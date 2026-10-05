#ifdef _PS

#include "ScreenPassShared.h"

// Must match DebugViewMode_e
#define DEBUGVIEW_LIT 0
#define DEBUGVIEW_ALBEDO 1
#define DEBUGVIEW_NORMAL 2
#define DEBUGVIEW_ROUGHNESS 3
#define DEBUGVIEW_METALLIC 4
#define DEBUGVIEW_SPECULAR 5
#define DEBUGVIEW_EMISSIVE 6
#define DEBUGVIEW_AO 7
#define DEBUGVIEW_DISTANCE_FIELD_AO 8

struct DebugViewUniforms_s
{
    uint Mode;
    uint SceneColorMetallicTextureIndex;
    uint SceneNormalRoughnessTextureIndex;
    uint SceneEmissiveSpecularTextureIndex;

    uint SceneDepthTextureIndex;
    uint SceneAOTextureIndex;
    uint FrameID;
    float Time;

    uint DistanceFieldAOTextureIndex;
    float3 __Pad;
};

ConstantBuffer<DebugViewUniforms_s> c_DebugView : register(b0);
Texture2D<float4> t_tex2d_f4[8192] : register(t1, space0);
Texture2D<float> t_tex2d_f1[8192] : register(t1, space1);

// Colour targets are linear and the back buffer is UNORM, matching the tonemapper's output encoding
float3 EncodeGamma(float3 Linear)
{
    return pow(saturate(Linear), 1.0f / 2.2f);
}

void main(in Interpolants_s Input, out float4 Output : SV_TARGET)
{
    const int3 Pixel = int3(Input.SVPosition.xy, 0);

    const float Depth = t_tex2d_f4[c_DebugView.SceneDepthTextureIndex].Load(Pixel).r;
    const bool IsSky = Depth >= 1.0f;

    float3 Color = 0.0f;

    [branch]
    if (c_DebugView.Mode == DEBUGVIEW_EMISSIVE)
    {
        Color = EncodeGamma(t_tex2d_f4[c_DebugView.SceneEmissiveSpecularTextureIndex].Load(Pixel).rgb);
    }
    else if (!IsSky)
    {
        if (c_DebugView.Mode == DEBUGVIEW_ALBEDO)
        {
            Color = EncodeGamma(t_tex2d_f4[c_DebugView.SceneColorMetallicTextureIndex].Load(Pixel).rgb);
        }
        else if (c_DebugView.Mode == DEBUGVIEW_NORMAL)
        {
            Color = saturate(abs(normalize(t_tex2d_f4[c_DebugView.SceneNormalRoughnessTextureIndex].Load(Pixel).xyz)));
        }
        else if (c_DebugView.Mode == DEBUGVIEW_ROUGHNESS)
        {
            Color = saturate(t_tex2d_f4[c_DebugView.SceneNormalRoughnessTextureIndex].Load(Pixel).aaa);
        }
        else if (c_DebugView.Mode == DEBUGVIEW_METALLIC)
        {
            Color = saturate(t_tex2d_f4[c_DebugView.SceneColorMetallicTextureIndex].Load(Pixel).aaa);
        }
        else if (c_DebugView.Mode == DEBUGVIEW_SPECULAR)
        {
            Color = saturate(t_tex2d_f4[c_DebugView.SceneEmissiveSpecularTextureIndex].Load(Pixel).aaa);
        }
        else if(c_DebugView.Mode == DEBUGVIEW_AO)
        {
            Color = saturate(t_tex2d_f1[c_DebugView.SceneAOTextureIndex].Load(Pixel).r);
        }
        else if (c_DebugView.Mode == DEBUGVIEW_DISTANCE_FIELD_AO)
        {
            Color = saturate(t_tex2d_f1[c_DebugView.DistanceFieldAOTextureIndex].Load(Pixel).r);
        }
    }

    Output = float4(Color, 1.0f);
}

#endif // #ifdef _PS
