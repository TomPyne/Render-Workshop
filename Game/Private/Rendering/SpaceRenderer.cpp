#include "Rendering/SpaceRenderer.h"

#include "Object/CameraComponent.h"
#include "Object/ObjectComponent.h"
#include "Rendering/IRenderable.h"
#include "Space/Space.h"
#include <Render/Render.h>
#include <RenderUtils/GPUContext/GPUContext.h>
#include <RenderUtils/RenderPasses/Tonemapping.h>
#include <Shared/Logging/Logging.h>

#include <SurfMath.h>

static struct SpaceRendererPrivate_s
{
	rl::RootSignaturePtr RootSignature;
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

	G.Initialized = true;
}

void SpaceRenderer_c::RenderSpace(const SpaceRendererScreenInfo_s& Screen, Space_c* Space, rl::CommandListSubmissionGroup& clGroup)
{
	if (!Space)
		return;

	auto Cam = Space->PrimaryCamera.lock();
	if (!Cam)
		return;

	matrix ProjectionMatrix = Cam->CalculateProjectionMatrix(Screen.Width, Screen.Height);
	matrix ViewMatrix = Cam->CalculateViewMatrix();

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

	SpaceViewUniforms_s ViewUniforms = {};
	ViewUniforms.ViewProjection = ViewMatrix * ProjectionMatrix;

	rl::DynamicBuffer_t ViewUniformsBuffer = rl::CreateDynamicConstantBuffer(&ViewUniforms);

	RenderGraphBuilder_s RGBuilder(RenderGraphResourcePool);

	RenderGraphResourceHandle_t SceneColorTexture = RGBuilder.CreateTexture(Screen.Width, Screen.Height, rl::RenderFormat::R16G16B16A16_FLOAT, RenderGraphResourceAccessType_e::RTV | RenderGraphResourceAccessType_e::SRV, L"SceneColorTexture");
	RenderGraphResourceHandle_t SceneDepthTexture = RGBuilder.CreateTexture(Screen.Width, Screen.Height, rl::RenderFormat::R32_FLOAT, RenderGraphResourceAccessType_e::DSV | RenderGraphResourceAccessType_e::SRV, L"SceneDepthTexture");

	RenderGraphPass_s& MeshDrawPass = RGBuilder.AddPass(RenderGraphPassType_e::GRAPHICS, L"Mesh Pass")
	.AccessResource(SceneColorTexture, RenderGraphResourceAccessType_e::RTV, RenderGraphLoadOp_e::CLEAR)
	.AccessResource(SceneDepthTexture, RenderGraphResourceAccessType_e::DSV, RenderGraphLoadOp_e::CLEAR)
	.SetExecuteCallback([=, &Collector](RenderGraph_s& RG, GPUContext_s& Ctx)
	{
		Ctx.SetRootSignature(G.RootSignature);
		rl::RenderTargetView_t SceneRTVs[] =
		{
			RG.GetRTV(SceneColorTexture),
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

	RenderGraphResourceHandle_t BackBufferTexture = RGBuilder.RefBackBufferTexture(Screen.RenderView->GetCurrentBackBufferTexture(), Screen.RenderView->GetCurrentBackBufferRTV(), rl::ResourceTransitionState::RENDER_TARGET, Screen.RenderView->Width, Screen.RenderView->Height);

	G.TonemapRenderer.AddPass(RGBuilder, TonemapMode_e::ACES, SceneColorTexture, BackBufferTexture);

	RenderGraph_s Graph = RGBuilder.Build();

	Graph.Execute(&clGroup);
}

rl::RootSignature_t SpaceRenderer_c::GetRootSignature()
{
	ASSERTMSG(G.Initialized, "SpaceRenderer has not been initialized");

	return G.RootSignature;
}
