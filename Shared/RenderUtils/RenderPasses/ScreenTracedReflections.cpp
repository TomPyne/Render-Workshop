#include "ScreenTracedReflections.h"

#include <imgui.h>
#include <Render/Render.h>
#include <RenderUtils/GPUContext/GPUContext.h>

struct STRUniforms_s
{
	matrix Projection;
	matrix InverseProjection;
	matrix View;

	uint32_t DepthTextureIndex;
	uint32_t SceneColorTextureIndex;
	uint32_t NormalTextureIndex;
	uint32_t OutputTextureIndex;

	float2 ViewportSizeRcp;
	uint2 ViewportSize;

	float Thickness;
	float MaxDistance;
	float MaxSteps;
	float Stride;

	float2 DepthProjection;
	float Jitter;
	float NearPlane;
};

void ScreenTracedReflectionRenderer_s::Init(uint32_t InUAVTableSlot, uint32_t InSRVTableSlot, uint32_t InCBVRootSlot, uint32_t InCBVSlot)
{
	UAVTableSlot = InUAVTableSlot;
	SRVTableSlot = InSRVTableSlot;
	CBVRootSlot = InCBVRootSlot;

	rl::ShaderMacros Macros = { { "CBV_SLOT", InCBVSlot } };

	rl::ComputePipelineStateDesc PSODesc = {};

	PSODesc.Cs = rl::CreateComputeShader("Shared/Shaders/STReflections/STReflections.hlsl", Macros);
	PSODesc.DebugName = L"ScreenSpaceReflectionCS";
	STRPSO = rl::CreateComputePipelineState(PSODesc);

	PSODesc.Cs = rl::CreateComputeShader("Shared/Shaders/STReflections/STReflectionCombine.hlsl", Macros);
	PSODesc.DebugName = L"ScreenSpaceReflectionCombineCS";
	STRCombinePSO = rl::CreateComputePipelineState(PSODesc);

	Ready = STRPSO.IsValid() && STRCombinePSO.IsValid();
}

RenderGraphResourceHandle_t ScreenTracedReflectionRenderer_s::GenerateSTRTexture(
	RenderGraphBuilder_s& RGBuilder,
	RenderGraphResourceHandle_t SceneDepth,
	RenderGraphResourceHandle_t SceneColor,
	RenderGraphResourceHandle_t SceneNormal,
	const matrix& Projection,
	const matrix& View,
	uint2 ScreenDim,
	float NearPlane)
{
	RenderGraphResourceHandle_t STRTexture = RGBuilder.CreateTexture(ScreenDim.x, ScreenDim.y, rl::RenderFormat::R8G8B8A8_UNORM, RenderGraphResourceAccessType_e::UAV | RenderGraphResourceAccessType_e::SRV, L"STReflectionTexture");

	RGBuilder.AddPass(RenderGraphPassType_e::COMPUTE, L"ST Reflection Pass")
	.AccessResource(SceneDepth, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(SceneColor, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(SceneNormal, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(STRTexture, RenderGraphResourceAccessType_e::UAV, RenderGraphLoadOp_e::DONT_CARE)
	.SetExecuteCallback([=](RenderGraph_s& RG, GPUContext_s& Ctx)
	{
		STRUniforms_s Uniforms;
		Uniforms.Projection = Projection;
		Uniforms.InverseProjection = InverseMatrix(Projection);
		Uniforms.View = View;
		Uniforms.DepthTextureIndex = RG.GetSRVIndex(SceneDepth);
		Uniforms.SceneColorTextureIndex = RG.GetSRVIndex(SceneColor);
		Uniforms.NormalTextureIndex = RG.GetSRVIndex(SceneNormal);
		Uniforms.OutputTextureIndex = RG.GetUAVIndex(STRTexture);
		Uniforms.ViewportSizeRcp = float2(1.0f / (float)ScreenDim.x, 1.0f / (float)ScreenDim.y);
		Uniforms.ViewportSize = ScreenDim;
		Uniforms.Thickness = Thickness;
		Uniforms.MaxDistance = MaxDistance;
		Uniforms.MaxSteps = (float)MaxSteps;
		Uniforms.Stride = Stride;
		Uniforms.DepthProjection = float2(Projection._33, Projection._43);
		Uniforms.Jitter = Jitter;
		Uniforms.NearPlane = NearPlane;

		Ctx.SetRootSignature();
		Ctx.SetComputeRootDescriptorTable(UAVTableSlot);
		Ctx.SetComputeRootDescriptorTable(SRVTableSlot);
		Ctx.SetComputeRootCBV(CBVRootSlot, rl::CreateDynamicConstantBuffer(&Uniforms));

		Ctx.SetPipelineState(STRPSO);

		Ctx.Dispatch(DivideRoundUp(ScreenDim.x, 8u), DivideRoundUp(ScreenDim.y, 8u), 1u);
	});

	return STRTexture;
}

void ScreenTracedReflectionRenderer_s::CombineSTR(RenderGraphBuilder_s& RGBuilder, RenderGraphResourceHandle_t SceneLit, RenderGraphResourceHandle_t SceneDepth, RenderGraphResourceHandle_t SceneNormal, RenderGraphResourceHandle_t SceneRoughnessMetallic, RenderGraphResourceHandle_t Reflection, const matrix& InverseViewProjection, const float3& CamPosition, uint2 ScreenDim)
{
	RGBuilder.AddPass(RenderGraphPassType_e::COMPUTE, L"ST Reflection Combine Pass")	
	.AccessResource(SceneDepth, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(SceneNormal, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(SceneRoughnessMetallic, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(Reflection, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(SceneLit, RenderGraphResourceAccessType_e::UAV, RenderGraphLoadOp_e::LOAD)
	.SetExecuteCallback([=](RenderGraph_s& RG, GPUContext_s& Ctx)
	{
		struct CombineUniforms_s
		{
			matrix InverseViewProjection;

			uint32_t SceneColorTextureIndex;
			uint32_t SceneDepthTextureIndex;
			uint32_t SceneNormalTextureIndex;
			uint32_t SceneRoughnessMetallicTextureIndex;

			uint32_t ReflectionTextureIndex;
			float3 CamPosition;

			uint2 ScreenDim;
			float2 ScreenDimRcp;
			float __Pad0[2];

		} Uniforms;

		Uniforms.InverseViewProjection = InverseViewProjection;
		Uniforms.SceneColorTextureIndex = RG.GetUAVIndex(SceneLit);
		Uniforms.SceneDepthTextureIndex = RG.GetSRVIndex(SceneDepth);
		Uniforms.SceneNormalTextureIndex = RG.GetSRVIndex(SceneNormal);
		Uniforms.SceneRoughnessMetallicTextureIndex = RG.GetSRVIndex(SceneRoughnessMetallic);
		Uniforms.ReflectionTextureIndex = RG.GetSRVIndex(Reflection);
		Uniforms.CamPosition = CamPosition;
		Uniforms.ScreenDim = ScreenDim;
		Uniforms.ScreenDimRcp = float2(1.0f / (float)ScreenDim.x, 1.0f / (float)ScreenDim.y);

		Ctx.SetRootSignature();
		Ctx.SetComputeRootDescriptorTable(UAVTableSlot);
		Ctx.SetComputeRootDescriptorTable(SRVTableSlot);
		Ctx.SetComputeRootCBV(CBVRootSlot, rl::CreateDynamicConstantBuffer(&Uniforms));
		Ctx.SetPipelineState(STRCombinePSO);

		Ctx.Dispatch(DivideRoundUp(ScreenDim.x, 8u), DivideRoundUp(ScreenDim.y, 8u), 1u);
	});
}

void ScreenTracedReflectionRenderer_s::DrawImGuiMenu()
{
	if (!MenuOpen)
		return;

	if (ImGui::Begin("STR Renderer", &MenuOpen))
	{
		ImGui::SliderFloat("Thickness", &Thickness, 0.0f, 5.0f);
		ImGui::SliderFloat("Max Distance", &MaxDistance, 0.0f, 500.0f);
		ImGui::SliderInt("Max Steps", &MaxSteps, 1, 100);
		ImGui::SliderFloat("Stride", &Stride, 0.1f, 5.0f);
		ImGui::SliderFloat("Jitter", &Jitter, 0.0f, 1.0f);

		if (ImGui::Button("Reset"))
		{
			Thickness = 0.01f;
			MaxDistance = 100.0f;
			MaxSteps = 500;
			Stride = 4.0f;
			Jitter = 3.0f;
		}
	}
	ImGui::End();
}
