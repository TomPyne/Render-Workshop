struct MaterialUniforms_s
{   
    float3 Color;
    float FadeInBrightness;

    float FadeInFalloff;
    float OpacityMaster;
    float DistanceSteadyWave;
    float ScaleSteadyWave;

    float2 OffsetSteadyWave;
    float SpeedSteadyWave;
    float Scale;

    float ScaleAspectRatio;
    float Speed;
    float2 Offset;

    uint AlbedoTexture;
    uint NormalTexture;
    float2 __Pad;
};

#define FORWARD_LIT 1

#include "../../../Game/Shaders/MeshMaterial.h"

struct Interpolants_s
{
    float4 Position : SV_POSITION;
    float4 PrevPosition : PREV_POSITION;
    float2 UV0 : TEXCOORD0;
    float2 UV1 : TEXCOORD1;
};

#include "SunTempleShared.h"

#ifdef _VS

void main(in uint VertexID : SV_VertexID, out Interpolants_s Output)
{
    const float3 ModelPosition = LoadPosition(VertexID);
    Output.Position = ModelToClip(ModelPosition);
    Output.PrevPosition = ModelToPrevClip(ModelPosition);
    Output.UV0 = LoadUV0(VertexID);
    Output.UV1 = LoadUV1(VertexID);
}

#endif // #ifdef _VS

#ifdef _PS

#include "../../../Game/Shaders/Samplers.h"
#include "SunTempleShared.h"

Texture2D<float4> t_tex2d_f4[8192] : register(t1, space0);

void main(in Interpolants_s Input, out PSOutput_s Output)
{
    float FadeInSample =  t_tex2d_f4[c_Material.NormalTexture].Sample(SharedWrappedSampler, Input.UV0).g;
    float FadeIn = saturate(pow(FadeInSample, c_Material.FadeInFalloff) * c_Material.FadeInBrightness);

    float2 Wave1UV = Input.UV1;
    Wave1UV *= float2(c_Material.ScaleAspectRatio, 1.0f) * c_Material.ScaleSteadyWave;
    Wave1UV += c_Material.OffsetSteadyWave;
    float Wave1Time = fmod(c_View.Time, 24.0f) * c_Material.SpeedSteadyWave;
    Wave1Time = sin(Wave1Time) * c_Material.DistanceSteadyWave;
    Wave1UV = Panner2(Wave1UV, Wave1Time, float2(0.05f, -0.5f));

    float2 Wave2UV = Input.UV1;
    Wave2UV *= float2(c_Material.ScaleAspectRatio, 1.0f) * c_Material.Scale;

    float2 WaveOffset = c_Material.Offset + Wave2UV;

    float Wave2Time = fmod(c_View.Time, 24.0f) * c_Material.Speed;

    float2 Wave2UVa = Panner2(Wave2UV, Wave2Time, float2(0.01f, -0.5f));
    float2 Wave2UVb = Panner2(Wave2UV * float2(0.25f, 1.0f), Wave2Time, float2(0.01f, -0.5f));

    float2 Wave3UVa = Panner2(WaveOffset, Wave2Time, float2(0.01f, -0.5f));
    float2 Wave3UVb = Panner2(WaveOffset * float2(0.25f, 1.0f), Wave2Time, float2(0.01f, -0.5f));

    float3 Wave1Albedo = t_tex2d_f4[c_Material.AlbedoTexture].Sample(SharedWrappedSampler, Wave1UV).rgb;
    float Wave1Normal = t_tex2d_f4[c_Material.NormalTexture].Sample(SharedWrappedSampler, Wave1UV).r;

    float3 Wave2Albedo = t_tex2d_f4[c_Material.AlbedoTexture].Sample(SharedWrappedSampler, Wave2UVa).rgb;
    float Wave2Normalr = t_tex2d_f4[c_Material.NormalTexture].Sample(SharedWrappedSampler, Wave2UVa).r;
    float Wave2Normalb = t_tex2d_f4[c_Material.NormalTexture].Sample(SharedWrappedSampler, Wave2UVb).b;

    float Wave2Normal = Wave2Normalr * Wave2Normalb;

    float3 Wave3Albedo = t_tex2d_f4[c_Material.AlbedoTexture].Sample(SharedWrappedSampler, Wave3UVa).rgb;
    float Wave3Normalr = t_tex2d_f4[c_Material.NormalTexture].Sample(SharedWrappedSampler, Wave3UVa).r;
    float Wave3Normalb = t_tex2d_f4[c_Material.NormalTexture].Sample(SharedWrappedSampler, Wave3UVb).b;

    float Wave3Normal = Wave3Normalr * Wave3Normalb;

    float Opacity = Wave2Normal + Wave3Normal + (Wave1Normal * FadeInSample);
    Opacity = saturate(Opacity * FadeIn) * c_Material.OpacityMaster;

    float3 Albedo = Wave2Albedo + Wave3Albedo + (Wave1Albedo * FadeInSample);
    Albedo *= c_Material.Color;

    MaterialOutput_Translucent(float4(Albedo, Opacity), Output);
}

#endif

