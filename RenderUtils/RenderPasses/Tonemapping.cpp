#include "Tonemapping.h"

#include "RenderUtils/GPUContext/GPUContext.h"

#include <Render/Render.h>

void TonemapRenderer_s::Init(rl::RootSignature_t InRootSignaure, uint32_t InCBVRootSlot, uint32_t InCBVSlot, uint32_t InSRVTableRootSigSlot)
{
	RootSignaure = InRootSignaure;
	SRVTableRootSigSlot = InSRVTableRootSigSlot;
	CBVRootSigSlot = InCBVRootSlot;
	std::string CBVSlotDef = "b" + std::to_string(InCBVSlot);
	rl::ShaderMacro BindingMacro = rl::ShaderMacro("CBV_SLOT", CBVSlotDef.c_str());

	rl::VertexShader_t ScreenPassVS = rl::CreateVertexShader("RenderUtils/Shaders/ScreenPass/ScreenPassVS.hlsl");
	rl::PixelShader_t NoTonemapPS = rl::CreatePixelShader("RenderUtils/Shaders/Tonemapping/Tonemapping.hlsl", { BindingMacro, { "NOTONEMAPPER" } });
	rl::PixelShader_t ACESTonemapPS = rl::CreatePixelShader("RenderUtils/Shaders/Tonemapping/Tonemapping.hlsl", { BindingMacro, { "ACESTONEMAPPER" } });
	rl::PixelShader_t FilmicTonemapPS = rl::CreatePixelShader("RenderUtils/Shaders/Tonemapping/Tonemapping.hlsl", { BindingMacro, { "FILMICTONEMAPPER" } });

	rl::GraphicsPipelineStateDesc PsoDesc = {};
	PsoDesc.RasterizerDesc(rl::PrimitiveTopologyType::TRIANGLE, rl::FillMode::SOLID, rl::CullMode::BACK)
		.DepthDesc(false)
		.TargetBlendDesc({ rl::RenderFormat::R8G8B8A8_UNORM }, { rl::BlendMode::None() }, rl::RenderFormat::UNKNOWN)
		.VertexShader(ScreenPassVS)
		.RootSignature(RootSignaure);

	PsoDesc.PixelShader(NoTonemapPS);
	PsoDesc.DebugName = L"NoTonemapPSO";
	NoTonemapPSO = rl::CreateGraphicsPipelineState(PsoDesc);

	PsoDesc.PixelShader(ACESTonemapPS);
	PsoDesc.DebugName = L"ACESTonemapPSO";
	ACESTonemapPSO = rl::CreateGraphicsPipelineState(PsoDesc);

	PsoDesc.PixelShader(FilmicTonemapPS);
	PsoDesc.DebugName = L"FilmicTonemapPSO";
	FilmicTonemapPSO = rl::CreateGraphicsPipelineState(PsoDesc);

	Ready = true;
}

void TonemapRenderer_s::FullScreenPassVSPS(RenderGraph_s& RG, GPUContext_s& Ctx, RenderGraphResourceHandle_t Target, rl::GraphicsPipelineState_t PSO, rl::DynamicBuffer_t UniformBuffer)
{
	if (!Ready)
		return;

	Ctx.SetRootSignature(RootSignaure);

	rl::RenderTargetView_t BackBufferRTV = RG.GetRTV(Target);
	const uint2 Dimensions = RG.GetTextureDimensions(Target); // TODO: this should be set by the graph

	Ctx.SetRenderTargets(&BackBufferRTV, 1, {}); // TODO: this should be set by the graph

	rl::Viewport vp{ Dimensions.x, Dimensions.y };
	Ctx.SetViewports(&vp, 1);
	Ctx.SetDefaultScissor(); // Could also be captured by the command context

	Ctx.SetGraphicsRootDescriptorTable(SRVTableRootSigSlot);
	Ctx.SetGraphicsRootCBV(CBVRootSigSlot, UniformBuffer);

	Ctx.SetPipelineState(PSO);

	Ctx.DrawInstanced(6u, 1u, 0u, 0u);
}

void TonemapRenderer_s::AddPass(RenderGraphBuilder_s& RGBuilder, TonemapMode_e Mode, RenderGraphResourceHandle_t Input, RenderGraphResourceHandle_t Output)
{
	if (Mode == TonemapMode_e::None)
	{
		RenderGraphPass_s& Pass = RGBuilder.AddPass(RenderGraphPassType_e::GRAPHICS, L"No Tonemapper")
		.AccessResource(Input, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
		.AccessResource(Output, RenderGraphResourceAccessType_e::RTV, RenderGraphLoadOp_e::DONT_CARE)
		.SetExecuteCallback([=](RenderGraph_s& RG, GPUContext_s& Ctx)
		{
			struct
			{
				uint32_t InputTexture;
				float3 __Pad;
			} Uniforms;

			Uniforms.InputTexture = RG.GetSRVIndex(Input);

			rl::DynamicBuffer_t TonemapCBuf = rl::CreateDynamicConstantBuffer(&Uniforms);

			FullScreenPassVSPS(RG, Ctx, Output, NoTonemapPSO, TonemapCBuf);
		});
	}
	else if (Mode == TonemapMode_e::ACES)
	{
		RenderGraphPass_s& Pass = RGBuilder.AddPass(RenderGraphPassType_e::GRAPHICS, L"ACES Tonemapper")
		.AccessResource(Input, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
		.AccessResource(Output, RenderGraphResourceAccessType_e::RTV, RenderGraphLoadOp_e::DONT_CARE)
		.SetExecuteCallback([=](RenderGraph_s& RG, GPUContext_s& Ctx)
		{
			struct
			{
				uint32_t InputTexture;
				float ExposureBias;
				float2 __Pad;
			} Uniforms;

			Uniforms.InputTexture = RG.GetSRVIndex(Input);
			Uniforms.ExposureBias = 1.0f;

			rl::DynamicBuffer_t TonemapCBuf = rl::CreateDynamicConstantBuffer(&Uniforms);

			FullScreenPassVSPS(RG, Ctx, Output, ACESTonemapPSO, TonemapCBuf);
		});
	}
	else if (Mode == TonemapMode_e::Filmic)
	{
		RenderGraphPass_s& Pass = RGBuilder.AddPass(RenderGraphPassType_e::GRAPHICS, L"Filmic Tonemapper")
		.AccessResource(Input, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
		.AccessResource(Output, RenderGraphResourceAccessType_e::RTV, RenderGraphLoadOp_e::DONT_CARE)
		.SetExecuteCallback([=](RenderGraph_s& RG, GPUContext_s& Ctx)
		{
			struct
			{
				uint32_t InputTexture;
				float ExposureBias;
				float WhitePoint;
				float __Pad;
			} Uniforms;

			Uniforms.InputTexture = RG.GetSRVIndex(Input);
			Uniforms.ExposureBias = 2.0f;
			Uniforms.WhitePoint = 11.2f;

			rl::DynamicBuffer_t TonemapCBuf = rl::CreateDynamicConstantBuffer(&Uniforms);

			FullScreenPassVSPS(RG, Ctx, Output, FilmicTonemapPSO, TonemapCBuf);
		});
	}
}
