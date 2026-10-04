#include "Rendering/DebugViewPass.h"

#include <Render/Render.h>
#include <RenderUtils/GPUContext/GPUContext.h>
#include <Shared/FileUtils/PathUtils.h>
#include <Shared/Logging/Logging.h>

const char* GetDebugViewModeName(DebugViewMode_e Mode)
{
	switch (Mode)
	{
	case DebugViewMode_e::Lit:			return "Lit";
	case DebugViewMode_e::Albedo:		return "Albedo";
	case DebugViewMode_e::Normal:		return "Normal (abs)";
	case DebugViewMode_e::Roughness:	return "Roughness";
	case DebugViewMode_e::Metallic:		return "Metallic";
	case DebugViewMode_e::Specular:		return "Specular";
	case DebugViewMode_e::Emissive:		return "Emissive";
	case DebugViewMode_e::Noise:		return "Noise";
	case DebugViewMode_e::GlobalDistanceFieldSlice:	return "Global Distance Field Slice";
	default:							return "Unknown";
	}
}

void DebugViewRenderer_s::Init(rl::RootSignature_t InRootSignature, uint32_t InCBVRootSigSlot, uint32_t InSRVTableRootSigSlot)
{
	RootSignature = rl::RootSignaturePtr::Ref(InRootSignature);
	CBVRootSigSlot = InCBVRootSigSlot;
	SRVTableRootSigSlot = InSRVTableRootSigSlot;

	static const Path_s ScreenPassVSPath = Path_s(PathDirectory_e::Shaders, L"Game", L"ScreenPassVS.hlsl");
	static const Path_s DebugViewPSPath = Path_s(PathDirectory_e::Shaders, L"Game", L"DebugView.hlsl");

	rl::GraphicsPipelineStateDesc PsoDesc = {};
	PsoDesc.RasterizerDesc(rl::PrimitiveTopologyType::TRIANGLE, rl::FillMode::SOLID, rl::CullMode::BACK)
		.DepthDesc(false)
		.TargetBlendDesc({ rl::RenderFormat::R8G8B8A8_UNORM }, { rl::BlendMode::None() }, rl::RenderFormat::UNKNOWN)
		.VertexShader(rl::CreateVertexShader(ScreenPassVSPath.ToString().c_str()))
		.PixelShader(rl::CreatePixelShader(DebugViewPSPath.ToString().c_str()))
		.RootSignature(RootSignature);

	PsoDesc.DebugName = L"DebugViewPSO";

	PSO = rl::CreateGraphicsPipelineState(PsoDesc);

	ASSERTMSG(PSO.IsValid(), "Failed to create Debug View PSO");
}

void DebugViewRenderer_s::AddPass(RenderGraphBuilder_s& RGBuilder, DebugViewMode_e Mode, const DebugViewInputs_s& Inputs, RenderGraphResourceHandle_t Output)
{
	RGBuilder.AddPass(RenderGraphPassType_e::GRAPHICS, L"Debug View Pass")
	.AccessResource(Inputs.SceneColorMetallic, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(Inputs.SceneNormalRoughness, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(Inputs.SceneEmissiveSpecular, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(Inputs.SceneDepth, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(Output, RenderGraphResourceAccessType_e::RTV, RenderGraphLoadOp_e::DONT_CARE)
	.SetExecuteCallback([=, this](RenderGraph_s& RG, GPUContext_s& Ctx)
	{
		struct DebugViewUniforms_s
		{
			uint32_t Mode;
			uint32_t SceneColorMetallicTextureIndex;
			uint32_t SceneNormalRoughnessTextureIndex;
			uint32_t SceneEmissiveSpecularTextureIndex;

			uint32_t SceneDepthTextureIndex;
			uint32_t BlueNoiseTextureIndex;
			uint32_t FrameID;
			float Time;
		};
		static_assert(sizeof(DebugViewUniforms_s) == 32, "Must match DebugViewUniforms_s in DebugView.hlsl");

		DebugViewUniforms_s Uniforms = {};
		Uniforms.Mode = static_cast<uint32_t>(Mode);
		Uniforms.SceneColorMetallicTextureIndex = RG.GetSRVIndex(Inputs.SceneColorMetallic);
		Uniforms.SceneNormalRoughnessTextureIndex = RG.GetSRVIndex(Inputs.SceneNormalRoughness);
		Uniforms.SceneEmissiveSpecularTextureIndex = RG.GetSRVIndex(Inputs.SceneEmissiveSpecular);
		Uniforms.SceneDepthTextureIndex = RG.GetSRVIndex(Inputs.SceneDepth);
		Uniforms.BlueNoiseTextureIndex = Inputs.BlueNoiseSRVIndex;
		Uniforms.FrameID = Inputs.Frame;
		Uniforms.Time = Inputs.Time;

		Ctx.SetRootSignature(RootSignature);

		rl::RenderTargetView_t RTV = RG.GetRTV(Output);
		const uint2 Dimensions = RG.GetTextureDimensions(Output);

		Ctx.SetRenderTargets(&RTV, 1, {}); // TODO: this should be set by the graph

		rl::Viewport vp{ Dimensions.x, Dimensions.y };
		Ctx.SetViewports(&vp, 1);
		Ctx.SetDefaultScissor();

		Ctx.SetGraphicsRootCBV(CBVRootSigSlot, RG.Alloc(Uniforms));
		Ctx.SetGraphicsRootDescriptorTable(SRVTableRootSigSlot);

		Ctx.SetPipelineState(PSO);

		Ctx.DrawInstanced(6u, 1u, 0u, 0u);
	});
}
