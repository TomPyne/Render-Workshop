#ifndef LUMINANCE
#define LUMINANCE 0
#endif

#ifndef WEIGHTS
#define WEIGHTS 0
#endif

#ifndef COPY
#define COPY 0
#endif

#ifndef BLEND
#define BLEND 0
#endif

#ifndef BLEND_LAPLACIAN
#define BLEND_LAPLACIAN 0
#endif

#ifndef FINAL_COMBINE
#define FINAL_COMBINE 0
#endif

float3 RRTAndODTFit(float3 V)
{
	float3 A = V * (V + 0.0245786) - 0.000090537;
	float3 B = V * (0.983729 * V + 0.4329510) + 0.238081;
	return A / B;
}

float3 ACESFilmicToneMapping(float3 Color) 
{
	const float ToneMappingExposure = 1.0f; // Could be parameterised

	const float3x3 ACESInputMat = float3x3(
	float3(0.59719, 0.07600, 0.02840),
	float3(0.35458, 0.90834, 0.13383),
	float3(0.04823, 0.01566, 0.83777));

	const float3x3 ACESOutputMat = float3x3(
	float3( 1.60475, -0.10208, -0.00327),
	float3(-0.53108,  1.10813, -0.07276),
	float3(-0.07367, -0.00605,  1.07602));

	Color *= ToneMappingExposure / 0.6;
	Color = mul(ACESInputMat, Color);
	Color = RRTAndODTFit(Color);
	Color = mul(ACESOutputMat, Color);
	return saturate(Color);
}

#if LUMINANCE

struct Uniforms
{
	float Exposure;
	float Shadows;
	float Highlights;
	uint InputTextureIndex;

	uint OutputTextureIndex;
	uint2 TextureSize;
	uint __Pad0;
};

ConstantBuffer<Uniforms> c_G : register(b1);

Texture2D<float4> t_tex2d_f4[8192] : register(t0, space0);
RWTexture2D<float4> u_tex2d_f4[8192] : register(u0, space0);

float SyntheticTonemap(float3 Color)
{
	float3 TonemappedColor = ACESFilmicToneMapping(Color);
	float Luminance = dot(TonemappedColor, float3(0.1, 0.7, 0.2));
	return sqrt(Luminance);
}

[NumThreads(8, 8, 1)]
void main(uint3 DispatchThreadId : SV_DispatchThreadID)
{
    if(any(DispatchThreadId.xy > c_G.TextureSize))
        return;

	const uint2 Pixel = DispatchThreadId.xy;
	
	float3 InputColor = t_tex2d_f4[c_G.InputTextureIndex][Pixel].rgb;

	float Highlights = SyntheticTonemap(InputColor * c_G.Highlights);
	float Midtones = SyntheticTonemap(InputColor);
	float Shadows = SyntheticTonemap(InputColor * c_G.Shadows);
	float3 Cols = float3(Highlights, Midtones, Shadows);

	u_tex2d_f4[c_G.OutputTextureIndex][Pixel] =  float4(Cols, 1.0f);
}

#endif // LUMINANCE

#if WEIGHTS

struct Uniforms
{
	uint DiffuseTextureIndex;
	uint OutputTextureIndex;
	uint2 TextureSize;
	
	float SigmaSq;
	float3 __Pad0;
};

ConstantBuffer<Uniforms> c_G : register(b1);

Texture2D<float4> t_tex2d_f4[8192] : register(t0, space0);
RWTexture2D<float4> u_tex2d_f4[8192] : register(u0, space0);

[NumThreads(8, 8, 1)]
void main(uint3 DispatchThreadId : SV_DispatchThreadID)
{
	if(any(DispatchThreadId.xy > c_G.TextureSize))
        return;

	const uint2 Pixel = DispatchThreadId.xy;

	float3 Diff = t_tex2d_f4[c_G.DiffuseTextureIndex][Pixel].rgb - float(0.5f).rrr;
	float3 Weights = float3(exp(-0.5f * Diff * Diff * c_G.SigmaSq));
	Weights /= Weights.r + Weights.g + Weights.b + 0.00001f;

	u_tex2d_f4[c_G.OutputTextureIndex][Pixel] = float4(Weights, 1.0f);
}

#endif // WEIGHTS

#if GENERATE_MIPS

struct Uniforms
{
	uint InputMipIndex;
	uint OutputMipIndex;
	uint2 OutputTextureSize;
};

ConstantBuffer<Uniforms> c_G : register(b1);

RWTexture2D<float4> u_tex2d_f4[8192] : register(u0, space0);

[NumThreads(8, 8, 1)]
void main(uint3 DispatchThreadId : SV_DispatchThreadID)
{
	if(any(DispatchThreadId.xy > c_G.OutputTextureSize))
        return;

	const uint2 Pixel = DispatchThreadId.xy;

	float4 V = u_tex2d_f4[c_G.InputMipIndex][Pixel * 2];
	V += u_tex2d_f4[c_G.InputMipIndex][Pixel * 2 + uint2(1, 0)];
	V += u_tex2d_f4[c_G.InputMipIndex][Pixel * 2 + uint2(1, 1)];
	V += u_tex2d_f4[c_G.InputMipIndex][Pixel * 2 + uint2(0, 1)];

	u_tex2d_f4[c_G.OutputMipIndex][Pixel] = V * 0.25f;
}

#endif // GENERATE_MIPS

#if BLEND

struct Uniforms
{
	uint ExposuresTextureIndex;
	uint WeightsTextureIndex;
	uint2 TextureSize;

	uint OutputTextureIndex;
	uint MipToUse;
	float2 __Pad0;
};

ConstantBuffer<Uniforms> c_G : register(b1);

Texture2D<float4> t_tex2d_f4[8192] : register(t0, space0);
RWTexture2D<float4> u_tex2d_f4[8192] : register(u0, space0);

[NumThreads(8, 8, 1)]
void main(uint3 DispatchThreadId : SV_DispatchThreadID)
{
	if(any(DispatchThreadId.xy > c_G.TextureSize))
        return;

	const uint2 Pixel = DispatchThreadId.xy;

	float3 Weights = t_tex2d_f4[c_G.WeightsTextureIndex].Load(int3(Pixel, c_G.MipToUse)).rgb;
	float3 Exposures = t_tex2d_f4[c_G.ExposuresTextureIndex].Load(uint3(Pixel, c_G.MipToUse)).rgb;
	Weights /= Weights.r + Weights.g + Weights.b + 0.0001f;

	float3 Output = Exposures * Weights;

	// TODO could be an F1 texture
	u_tex2d_f4[c_G.OutputTextureIndex][Pixel] = float4(float(Output.r + Output.g + Output.b).rrr, 1.0f);
}

#endif // BLEND

#if BLEND_LAPLACIAN

struct Uniforms
{
	uint ExposuresTextureIndex;
	uint WeightsTextureIndex;
	uint MipToUse;
	uint AccumSoFarTextureIndex;

	uint OutputTextureIndex;
	float BoostLocalContrast;
	uint2 TextureSize;
};

ConstantBuffer<Uniforms> c_G : register(b1);

Texture2D<float4> t_tex2d_f4[8192] : register(t0, space0);
RWTexture2D<float4> u_tex2d_f4[8192] : register(u0, space0);
SamplerState ClampedSampler : register(s1);

[NumThreads(8, 8, 1)]
void main(uint3 DispatchThreadId : SV_DispatchThreadID)
{
	if(any(DispatchThreadId.xy > c_G.TextureSize))
        return;

	const uint2 Pixel = DispatchThreadId.xy;
	const float2 UV = (float2(Pixel) + float2(0.5f, 0.5f)) / float2(c_G.TextureSize);

	// Blend Laplacians based on exposure weights
	float AccumSoFar = u_tex2d_f4[c_G.AccumSoFarTextureIndex][uint2(UV * (c_G.TextureSize / 2))].r;
	float3 Laplacians = t_tex2d_f4[c_G.ExposuresTextureIndex].SampleLevel(ClampedSampler, UV, c_G.MipToUse - 1).rgb - t_tex2d_f4[c_G.ExposuresTextureIndex].SampleLevel(ClampedSampler, UV, c_G.MipToUse).rgb;
	float3 Weights = t_tex2d_f4[c_G.WeightsTextureIndex].Load(int3(Pixel, c_G.MipToUse - 1)).rgb;

	if(c_G.BoostLocalContrast)
	{
		Weights *= abs(Laplacians) + 0.00001f;
	}

	Weights /= Weights.r + Weights.g + Weights.b + 0.00001f;
	float WeightedLaplacian = dot(Laplacians * Weights, float3(1.0f, 1.0f, 1.0f));

	// TODO: Could be an F1 texture
	u_tex2d_f4[c_G.OutputTextureIndex][Pixel] = float4(float(AccumSoFar + WeightedLaplacian).rrr, 1.0f);
}

#endif // BLEND_LAPLACIAN

#if FINAL_COMBINE

struct Uniforms
{
	uint DiffuseTextureIndex;
	uint OriginalMipTextureIndex;
	uint OutputTextureIndex;
	uint DisplayMip;

	uint2 TextureSize;
	float Exposure;	
	float __Pad0;
};

ConstantBuffer<Uniforms> c_G : register(b1);

Texture2D<float4> t_tex2d_f4[8192] : register(t0, space0);
RWTexture2D<float4> u_tex2d_f4[8192] : register(u0, space0);
SamplerState ClampedSampler : register(s1);


[NumThreads(8, 8, 1)]
void main(uint3 DispatchThreadId : SV_DispatchThreadID)
{
	if(any(DispatchThreadId.xy > c_G.TextureSize))
        return;

	const uint2 Pixel = DispatchThreadId.xy;

	// Guided upsampling.
	// See https://bartwronski.com/2019/09/22/local-linear-models-guided-filter/

	float MomentX = 0.0f;
	float MomentY = 0.0f;
	float MomentX2 = 0.0f;
	float MomentXY = 0.0f;
	float WS = 0.0f;

	for(int DY = -1; DY <= 1; DY += 1)
	{
		for(int DX = -1; DX <= 1; DX += 1)
		{
			const int2 SamplePixel = int2(Pixel) + int2(DX, DY);
			const float2 UV = (float2(SamplePixel) + float2(0.5f, 0.5f)) / float2(c_G.TextureSize);

			float X = t_tex2d_f4[c_G.OriginalMipTextureIndex].SampleLevel(ClampedSampler, UV, c_G.DisplayMip).y;
			float Y = t_tex2d_f4[c_G.DiffuseTextureIndex].SampleLevel(ClampedSampler, UV, c_G.DisplayMip).x;
			float W = exp(-0.5f * float(DX * DX + DY * DY) / (0.7 * 0.7));
			MomentX += X * W;
			MomentY += Y * W;
			MomentX2 += X * X * W;
			MomentXY += X * Y * W;
			WS += W;
		}
	}

	MomentX /= WS;
	MomentY /= WS;
	MomentX2 /= WS;
	MomentXY /= WS;

	float A = (MomentXY - MomentX * MomentY) / (max(MomentX2 - MomentX * MomentX, 0.0f) + 0.00001f);
	float B = MomentY - A * MomentX;

	// Apply local exposure adjustment as a crude multiplier on all RGB channels.
	// This is... generally pretty wrong, but enough for the demo purpose.
	float3 Texel = t_tex2d_f4[c_G.DiffuseTextureIndex].Load(uint3(Pixel, c_G.DisplayMip)).rgb;
	float3 Original = u_tex2d_f4[c_G.OutputTextureIndex][Pixel].rgb;
	float3 TexelOriginal = sqrt(max(ACESFilmicToneMapping(Original * c_G.Exposure), 0.0f));
	float Luminance = dot(TexelOriginal, float3(0.1f, 0.7f, 0.2f)) + 0.00001f;
	float FinalMultiplier = max(A * Luminance + B, 0.0f) / Luminance;

	// hAck to prevent super dark pixels getting boosted by a lot and showing compression artifacts.
	float LerpToUnityThreshold = 0.007f;
	if(Luminance <= LerpToUnityThreshold)
	{
		FinalMultiplier = lerp(1.0f, FinalMultiplier, (Luminance / LerpToUnityThreshold) * (Luminance / LerpToUnityThreshold));
	}

	float3 TexelFinal = sqrt(max(ACESFilmicToneMapping(Original * c_G.Exposure * FinalMultiplier), 0.0f));
	u_tex2d_f4[c_G.OutputTextureIndex][Pixel] = float4(TexelFinal, 1.0f);
	//u_tex2d_f4[c_G.OutputTextureIndex][Pixel] = float4(TexelOriginal, 1.0f);
}

#endif // FINAL_COMBINE