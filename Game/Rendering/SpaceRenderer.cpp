#include "SpaceRenderer.h"

#include "Game/Object/ObjectComponent.h"
#include "Game/Rendering/IRenderable.h"
#include "Game/Space/Space.h"
#include <Render/Render.h>
#include <RenderUtils/RenderGraph/RenderGraph.h>
#include <RenderUtils/GPUContext/GPUContext.h>

#include <SurfMath.h>

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

struct SpaceRenderGLobals_s
{
	RenderGraphResourcePool_s RenderGraphResourcePool;
} G;

void SpaceRenderer_c::RenderSpace(const SpaceRendererScreenInfo_s& Screen, Space_c* Space, rl::CommandListSubmissionGroup& clGroup)
{
	if (!Space)
		return;

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
	ViewUniforms.ViewProjection = Space->PrimaryView.ViewMatrix * Space->PrimaryView.ProjectionMatrix;

	rl::DynamicBuffer_t ViewUniformsBuffer = rl::CreateDynamicConstantBuffer(&ViewUniforms);

	RenderGraphBuilder_s RGBuilder(G.RenderGraphResourcePool);

	RenderGraphResourceHandle_t SceneColorTexture = RGBuilder.CreateTexture(Screen.Width, Screen.Height, rl::RenderFormat::R16G16B16A16_FLOAT, RenderGraphResourceAccessType_e::RTV | RenderGraphResourceAccessType_e::SRV, L"SceneColorTexture");
	RenderGraphResourceHandle_t SceneDepthTexture = RGBuilder.CreateTexture(Screen.Width, Screen.Height, rl::RenderFormat::R32_FLOAT, RenderGraphResourceAccessType_e::DSV | RenderGraphResourceAccessType_e::SRV, L"SceneDepthTexture");

	RenderGraphPass_s& MeshDrawPass = RGBuilder.AddPass(RenderGraphPassType_e::GRAPHICS, L"Mesh Pass")
	.AccessResource(SceneColorTexture, RenderGraphResourceAccessType_e::RTV, RenderGraphLoadOp_e::CLEAR)
	.AccessResource(SceneDepthTexture, RenderGraphResourceAccessType_e::DSV, RenderGraphLoadOp_e::CLEAR)
	.SetExecuteCallback([=, &Collector](RenderGraph_s& RG, GPUContext_s& Ctx)
	{
		Ctx.SetRootSignature();
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
			Ctx.SetGraphicsRootCBV(SpaceRendererRootSigSlots::RS_MODEL_BUF, Batch.MeshUniforms);
			Ctx.SetGraphicsRootCBV(SpaceRendererRootSigSlots::RS_MAT_BUF, Batch.MaterialUniforms);

			Ctx.SetIndexBuffer(Batch.IndexBuffer, Batch.IndexBufferFormat, Batch.IndexCount);
			Ctx.DrawIndexedInstanced(Batch.IndexCount, 1, 0, 0, 0);
		}
	});
}
