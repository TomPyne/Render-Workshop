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

	std::string CBVSlotDef = "b" + std::to_string(InCBVSlot);
	rl::ShaderMacros Macros = { { "CBV_SLOT", CBVSlotDef.c_str() } };

	rl::ComputePipelineStateDesc PSODesc = {};
	PSODesc.Cs = rl::CreateComputeShader("Shared/Shaders/STReflections/STReflections.hlsl", Macros);
	PSODesc.DebugName = L"ScreenSpaceReflectionCS";

	STRPSO = rl::CreateComputePipelineState(PSODesc);

	Ready = STRPSO != rl::ComputePipelineState_t::INVALID;
}

RenderGraphResourceHandle_t ScreenTracedReflectionRenderer_s::GenerateSTRTexture(
	RenderGraphBuilder_s& RGBuilder,
	RenderGraphResourceHandle_t SceneDepth,
	RenderGraphResourceHandle_t SceneColor,
	RenderGraphResourceHandle_t SceneNormal,
	const matrix& Projection,
	const matrix& PixelProjection,
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
