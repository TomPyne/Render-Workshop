#include "Rendering/SpaceRenderer.h"

#include "Object/CameraComponent.h"
#include "Object/ObjectComponent.h"
#include "Rendering/IRenderable.h"
#include "Space/Space.h"
#include <Render/Render.h>
#include <RenderUtils/GPUContext/GPUContext.h>
#include <RenderUtils/RenderPasses/Tonemapping.h>
#include <Shared/FileUtils/PathUtils.h>
#include <Shared/Logging/Logging.h>

#include <SurfMath.h>

static struct SpaceRendererPrivate_s
{
	rl::RootSignaturePtr RootSignature;
	rl::GraphicsPipelineStatePtr DeferredPSO;
	TonemapRenderer_s TonemapRenderer;
	bool Initialized = false;
} G;

namespace SpaceRendererRootSigSlots
{
	enum Value
	{
		RS_DRAWCONSTANTS,
		RS_VIEW_BUF,
		RS_MODEL_BUF,
		RS_MAT_BUF,
		RS_SRV_TABLE,
		RS_UAV_TABLE,
		RS_COUNT,
	};
}
struct SpaceViewUniforms_s
{
	matrix ViewProjection;

	float3 CamPos;
	float Time;

	float2 InvViewportSize;
	float2 Pad0;
};

void SpaceRenderer_c::Init()
{
	ASSERTMSG(G.Initialized == false, "Space Renderer has already been initialized");
	static const uint32_t DrawCBVRegister = 0;
	static const uint32_t ViewCBVRegister = 1;
	static const uint32_t ModelCBVRegister = 2;
	static const uint32_t MatCBVRegister = 3;

	rl::RootSignatureDesc RootSigDesc = {};
	RootSigDesc.Slots.resize(SpaceRendererRootSigSlots::RS_COUNT);
	RootSigDesc.Slots[SpaceRendererRootSigSlots::RS_DRAWCONSTANTS] = rl::RootSignatureSlot::CBVSlot(DrawCBVRegister, 0);
	RootSigDesc.Slots[SpaceRendererRootSigSlots::RS_VIEW_BUF] = rl::RootSignatureSlot::CBVSlot(ViewCBVRegister, 0);
	RootSigDesc.Slots[SpaceRendererRootSigSlots::RS_MODEL_BUF] = rl::RootSignatureSlot::CBVSlot(ModelCBVRegister, 0);
	RootSigDesc.Slots[SpaceRendererRootSigSlots::RS_MAT_BUF] = rl::RootSignatureSlot::CBVSlot(MatCBVRegister, 0);
	RootSigDesc.Slots[SpaceRendererRootSigSlots::RS_SRV_TABLE] = rl::RootSignatureSlot::DescriptorTableSlot(0, 0, rl::RootSignatureDescriptorTableType::SRV);
	RootSigDesc.Slots[SpaceRendererRootSigSlots::RS_UAV_TABLE] = rl::RootSignatureSlot::DescriptorTableSlot(0, 0, rl::RootSignatureDescriptorTableType::UAV);

	RootSigDesc.GlobalSamplers.resize(2);
	RootSigDesc.GlobalSamplers[0].AddressModeUVW(rl::SamplerAddressMode::WRAP).FilterModeMinMagMip(rl::SamplerFilterMode::ANISOTROPIC);
	RootSigDesc.GlobalSamplers[1].AddressModeUVW(rl::SamplerAddressMode::CLAMP).FilterModeMinMagMip(rl::SamplerFilterMode::LINEAR);

	G.RootSignature = rl::CreateRootSignature(RootSigDesc);

	G.TonemapRenderer.Init(G.RootSignature, SpaceRendererRootSigSlots::RS_VIEW_BUF, ViewCBVRegister, SpaceRendererRootSigSlots::RS_SRV_TABLE);

	static const Path_s ScreenPassVSPath = Path_s(PathDirectory_e::Shaders, L"Game", L"ScreenPassVS.hlsl");
	rl::VertexShader_t ScreenPassVS = rl::CreateVertexShader(ScreenPassVSPath.ToString().c_str());

	// Deferred PSO
	{
		static const Path_s DeferredPSPath = Path_s(PathDirectory_e::Shaders, L"Game", L"Deferred.hlsl");
		rl::PixelShader_t DeferredPS = rl::CreatePixelShader(DeferredPSPath.ToString().c_str());

		rl::GraphicsPipelineStateDesc PsoDesc = {};
		PsoDesc.RasterizerDesc(rl::PrimitiveTopologyType::TRIANGLE, rl::FillMode::SOLID, rl::CullMode::BACK)
			.DepthDesc(false)
			.TargetBlendDesc({ rl::RenderFormat::R11G11B10_FLOAT }, { rl::BlendMode::None() }, rl::RenderFormat::UNKNOWN)
			.VertexShader(ScreenPassVS)
			.PixelShader(DeferredPS)
			.RootSignature(G.RootSignature);

		PsoDesc.DebugName = L"DeferredPSO";

		G.DeferredPSO = CreateGraphicsPipelineState(PsoDesc);
	}

	Clock = {};

	G.Initialized = true;
}

void SpaceRenderer_c::RenderSpace(const SpaceRendererScreenInfo_s& Screen, Space_c* Space, rl::CommandListSubmissionGroup& clGroup)
{
	if (!Space)
		return;

	CameraComponent_c* PrimaryCamera = Space->GetCamera();
	if (!PrimaryCamera)
		return;

	matrix ProjectionMatrix = PrimaryCamera->CalculateProjectionMatrix(Screen.Width, Screen.Height);
	matrix ViewMatrix = PrimaryCamera->CalculateViewMatrix();

	SpatialRenderingCollector_s Collector = {};

	for (std::shared_ptr<Object_c>& Object : Space->Objects)
	{
		for (std::shared_ptr<ObjectComponent_c>& Component : Object->Components)
		{
			if (IRenderable_c* Renderable = dynamic_cast<IRenderable_c*>(Component.get()))
			{
				Renderable->Render(Collector);
			}
		}
	}

	Clock.Tick();

	SpaceViewUniforms_s ViewUniforms = {};
	ViewUniforms.ViewProjection = ViewMatrix * ProjectionMatrix;
	ViewUniforms.CamPos = PrimaryCamera->GetWorldPosition();
	ViewUniforms.Time = Clock.GetTotalSeconds();
	ViewUniforms.InvViewportSize = float2(1.0f / Screen.Width, 1.0f / Screen.Height);

	rl::DynamicBuffer_t ViewUniformsBuffer = rl::CreateDynamicConstantBuffer(&ViewUniforms);

	RenderGraphBuilder_s RGBuilder(RenderGraphResourcePool);

	RenderGraphResourceHandle_t SceneColorMetallicTexture = RGBuilder.CreateTexture(Screen.Width, Screen.Height, rl::RenderFormat::R16G16B16A16_FLOAT, RenderGraphResourceAccessType_e::RTV | RenderGraphResourceAccessType_e::SRV, L"SceneColorMetallicTexture");
	RenderGraphResourceHandle_t SceneNormalRoughnessTexture = RGBuilder.CreateTexture(Screen.Width, Screen.Height, rl::RenderFormat::R16G16B16A16_FLOAT, RenderGraphResourceAccessType_e::RTV | RenderGraphResourceAccessType_e::SRV, L"SceneNormalRoughnessTexture");
	RenderGraphResourceHandle_t SceneEmissiveSpecularTexture = RGBuilder.CreateTexture(Screen.Width, Screen.Height, rl::RenderFormat::R16G16B16A16_FLOAT, RenderGraphResourceAccessType_e::RTV | RenderGraphResourceAccessType_e::SRV, L"SceneEmissiveSpecularTexture");
	RenderGraphResourceHandle_t SceneDepthTexture = RGBuilder.CreateTexture(Screen.Width, Screen.Height, rl::RenderFormat::R32_FLOAT, RenderGraphResourceAccessType_e::DSV | RenderGraphResourceAccessType_e::SRV, L"SceneDepthTexture");

	RenderGraphPass_s& MeshDrawPass = RGBuilder.AddPass(RenderGraphPassType_e::GRAPHICS, L"Mesh Pass")
	.AccessResource(SceneColorMetallicTexture, RenderGraphResourceAccessType_e::RTV, RenderGraphLoadOp_e::CLEAR)
	.AccessResource(SceneNormalRoughnessTexture, RenderGraphResourceAccessType_e::RTV, RenderGraphLoadOp_e::CLEAR)
	.AccessResource(SceneEmissiveSpecularTexture, RenderGraphResourceAccessType_e::RTV, RenderGraphLoadOp_e::CLEAR)
	.AccessResource(SceneDepthTexture, RenderGraphResourceAccessType_e::DSV, RenderGraphLoadOp_e::CLEAR)
	.SetExecuteCallback([=, &Collector](RenderGraph_s& RG, GPUContext_s& Ctx)
	{
		Ctx.SetRootSignature(G.RootSignature);
		rl::RenderTargetView_t SceneRTVs[] =
		{
			RG.GetRTV(SceneColorMetallicTexture),
			RG.GetRTV(SceneNormalRoughnessTexture),
			RG.GetRTV(SceneEmissiveSpecularTexture),
		};

		rl::DepthStencilView_t SceneDSV = RG.GetDSV(SceneDepthTexture);
		Ctx.SetRenderTargets(SceneRTVs, ARRAYSIZE(SceneRTVs), SceneDSV); // TODO: this should be set by the graph

		rl::Viewport vp{ Screen.Width, Screen.Height };
		Ctx.SetViewports(&vp, 1);
		Ctx.SetDefaultScissor(); // Could also be captured by the command context

		Ctx.SetGraphicsRootCBV(SpaceRendererRootSigSlots::RS_VIEW_BUF, ViewUniformsBuffer);
		Ctx.SetGraphicsRootDescriptorTable(SpaceRendererRootSigSlots::RS_SRV_TABLE); // Root sig stuff is trickier

		for (const SpatialRenderingBatch_s& Batch : Collector.MainPass.Batches)
		{
			Ctx.SetPipelineState(Batch.PSO); // TODO: check when PSO has changed in the command list
			Ctx.SetGraphicsRootCBV(SpaceRendererRootSigSlots::RS_DRAWCONSTANTS, Batch.DynamicUniforms);
			Ctx.SetGraphicsRootCBV(SpaceRendererRootSigSlots::RS_MODEL_BUF, Batch.MeshUniforms);
			Ctx.SetGraphicsRootCBV(SpaceRendererRootSigSlots::RS_MAT_BUF, Batch.MaterialUniforms);

			Ctx.SetIndexBuffer(Batch.IndexBuffer, Batch.IndexBufferFormat, 0);
			Ctx.DrawIndexedInstanced(Batch.IndexCount, 1, 0, 0, 0);
		}
	});

	RenderGraphResourceHandle_t LitTexture = RGBuilder.CreateTexture(Screen.Width, Screen.Height, rl::RenderFormat::R11G11B10_FLOAT, RenderGraphResourceAccessType_e::RTV | RenderGraphResourceAccessType_e::SRV, L"LitTexture");

	RenderGraphPass_s& DeferredPass = RGBuilder.AddPass(RenderGraphPassType_e::GRAPHICS, L"Deferred Pass")
	.AccessResource(SceneColorMetallicTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(SceneNormalRoughnessTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(SceneEmissiveSpecularTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(SceneDepthTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(LitTexture, RenderGraphResourceAccessType_e::RTV, RenderGraphLoadOp_e::DONT_CARE)
	.SetExecuteCallback([=](RenderGraph_s& RG, GPUContext_s& Ctx)
	{
		struct DeferredConstants_s
		{
			matrix InvViewProjection;

			float3 LightDirection;
			uint32_t SceneColorMetallicTextureIndex;

			float3 LightRadiance;
			uint32_t SceneNormalRoughnessTextureIndex;

			float3 AmbientColor;
			uint32_t SceneEmissiveSpecularTextureIndex;

			uint32_t SceneDepthTextureIndex;
			float3 __Pad;
		} Uniforms;
		static_assert(sizeof(DeferredConstants_s) == 128, "Must match DeferredData_s in Deferred.hlsl");

		// TODO: replace with a light component
		Uniforms.InvViewProjection = InverseMatrix(ViewUniforms.ViewProjection);
		Uniforms.LightDirection = Normalize(float3(-0.5f, 1.0f, 0.5f));
		Uniforms.LightRadiance = float3(1.0f, 0.95f, 0.85f) * 3.0f;
		Uniforms.AmbientColor = float3(0.25f, 0.3f, 0.4f) * 0.3f;

		Uniforms.SceneColorMetallicTextureIndex = RG.GetSRVIndex(SceneColorMetallicTexture);
		Uniforms.SceneNormalRoughnessTextureIndex = RG.GetSRVIndex(SceneNormalRoughnessTexture);
		Uniforms.SceneEmissiveSpecularTextureIndex = RG.GetSRVIndex(SceneEmissiveSpecularTexture);
		Uniforms.SceneDepthTextureIndex = RG.GetSRVIndex(SceneDepthTexture);

		rl::DynamicBuffer_t DeferredCBuf = rl::CreateDynamicConstantBuffer(&Uniforms);

		Ctx.SetRootSignature(G.RootSignature);

		rl::RenderTargetView_t RTV = RG.GetRTV(LitTexture);

		Ctx.SetRenderTargets(&RTV, 1, {}); // TODO: this should be set by the graph

		rl::Viewport vp{ Screen.Width, Screen.Height };
		Ctx.SetViewports(&vp, 1);
		Ctx.SetDefaultScissor(); // Could also be captured by the command context

		Ctx.SetGraphicsRootCBV(SpaceRendererRootSigSlots::RS_VIEW_BUF, ViewUniformsBuffer);
		Ctx.SetGraphicsRootCBV(SpaceRendererRootSigSlots::RS_MODEL_BUF, DeferredCBuf);
		Ctx.SetGraphicsRootDescriptorTable(SpaceRendererRootSigSlots::RS_SRV_TABLE); // Root sig stuff is trickier

		Ctx.SetPipelineState(G.DeferredPSO);

		Ctx.DrawInstanced(6u, 1u, 0u, 0u);
	});

	RenderGraphResourceHandle_t BackBufferTexture = RGBuilder.RefBackBufferTexture(Screen.RenderView->GetCurrentBackBufferTexture(), Screen.RenderView->GetCurrentBackBufferRTV(), rl::ResourceTransitionState::RENDER_TARGET, Screen.RenderView->Width, Screen.RenderView->Height);

	G.TonemapRenderer.AddPass(RGBuilder, TonemapMode_e::ACES, LitTexture, BackBufferTexture);

	RenderGraph_s Graph = RGBuilder.Build();

	Graph.Execute(&clGroup);
}

rl::RootSignature_t SpaceRenderer_c::GetRootSignature()
{
	ASSERTMSG(G.Initialized, "SpaceRenderer has not been initialized");

	return G.RootSignature;
}

const rl::GraphicsPipelineTargetDesc& SpaceRenderer_c::GetMaterialPipelineTargetDesc()
{
	static rl::GraphicsPipelineTargetDesc MaterialPipelineTargetDesc = rl::GraphicsPipelineTargetDesc(
		{ 
			rl::RenderFormat::R16G16B16A16_FLOAT, // Albedo + Metallic
			rl::RenderFormat::R16G16B16A16_FLOAT, // Normal + Roughness
			rl::RenderFormat::R16G16B16A16_FLOAT, // Emissive + Specular
		}, 
		{ 
			rl::BlendMode::None(),
			rl::BlendMode::None(),
			rl::BlendMode::None(),
		}, 
		rl::RenderFormat::D32_FLOAT);
	return MaterialPipelineTargetDesc;
}
