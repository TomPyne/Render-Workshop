#include "DebugDrawPass.h"

#include "Rendering/SpaceRenderer.h"

#include <Render/Render.h>
#include <RenderUtils/GPUContext/GPUContext.h>
#include <Shared/FileUtils/PathUtils.h>
#include <Shared/Logging/Logging.h>

void DebugDrawRenderer_s::Init()
{
	static const Path_s DebugDrawPath = Path_s(PathDirectory_e::Shaders, L"Game", L"DebugDraw.hlsl");

	const rl::VertexShader_t VS = rl::CreateVertexShader(DebugDrawPath.ToString().c_str());
	const rl::PixelShader_t PS = rl::CreatePixelShader(DebugDrawPath.ToString().c_str());

	for (uint32_t PrimitiveIt = 0; PrimitiveIt < static_cast<uint32_t>(DebugPrimitive_e::Count); ++PrimitiveIt)
	{
		for (uint32_t ModeIt = 0; ModeIt < static_cast<uint32_t>(DebugDepthMode_e::Count); ++ModeIt)
		{
			const bool Lines = static_cast<DebugPrimitive_e>(PrimitiveIt) == DebugPrimitive_e::Lines;
			const bool DepthTested = static_cast<DebugDepthMode_e>(ModeIt) == DebugDepthMode_e::DepthTested;

			rl::GraphicsPipelineStateDesc PsoDesc = {};
			PsoDesc.RasterizerDesc(Lines ? rl::PrimitiveTopologyType::LINE : rl::PrimitiveTopologyType::TRIANGLE, rl::FillMode::SOLID, rl::CullMode::NONE)
				.DepthDesc(DepthTested, rl::ComparisionFunc::LESS_EQUAL, rl::DepthWriteMask::ZERO)
				.TargetBlendDesc({ rl::RenderFormat::R8G8B8A8_UNORM }, { rl::BlendMode::Default() }, SpaceRenderer_c::GetMaterialPipelineDepthFormat())
				.VertexShader(VS)
				.PixelShader(PS)
				.RootSignature(SpaceRenderer_c::GetRootSignature());

			PsoDesc.DebugName = Lines ? (DepthTested ? L"DebugDrawLinesPSO" : L"DebugDrawLinesOnTopPSO") : (DepthTested ? L"DebugDrawTrianglesPSO" : L"DebugDrawTrianglesOnTopPSO");

			PSOs[PrimitiveIt][ModeIt] = rl::CreateGraphicsPipelineState(PsoDesc);

			ASSERTMSG(PSOs[PrimitiveIt][ModeIt].IsValid(), "Failed to create Debug Draw PSO");
		}
	}
}

void DebugDrawRenderer_s::AddPass(RenderGraphBuilder_s& RGBuilder, const SpatialRenderingCollector_s& Collector, RenderGraphResourceHandle_t Target, RenderGraphResourceHandle_t SceneDepth,
	FrameBufferAlloc_s ViewUniforms, uint2 ScreenSize)
{
	struct DebugDrawStream_s
	{
		rl::GraphicsPipelineState_t PSO;
		FrameBufferAlloc_s Vertices;
		uint32_t VertexCount;
	};

	DebugDrawStream_s Streams[static_cast<size_t>(DebugPrimitive_e::Count) * static_cast<size_t>(DebugDepthMode_e::Count)];
	uint32_t NumStreams = 0;

	for (DebugPrimitive_e Primitive : { DebugPrimitive_e::Triangles, DebugPrimitive_e::Lines })
	{
		for (uint32_t ModeIt = 0; ModeIt < static_cast<uint32_t>(DebugDepthMode_e::Count); ++ModeIt)
		{
			const std::vector<DebugVertex_s>& Vertices = Collector.GetDebugVertices(Primitive, static_cast<DebugDepthMode_e>(ModeIt));
			if (Vertices.empty())
				continue;

			DebugDrawStream_s& Stream = Streams[NumStreams++];
			Stream.PSO = PSOs[static_cast<size_t>(Primitive)][ModeIt];
			Stream.Vertices = RGBuilder.GetMainFrameBuffer().AllocData(Vertices.data(), static_cast<uint32_t>(Vertices.size() * sizeof(DebugVertex_s)));
			Stream.VertexCount = static_cast<uint32_t>(Vertices.size());
		}
	}

	if (NumStreams == 0)
		return;

	RGBuilder.AddPass(RenderGraphPassType_e::GRAPHICS, L"Debug Draw Pass")
	.AccessResource(Target, RenderGraphResourceAccessType_e::RTV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(SceneDepth, RenderGraphResourceAccessType_e::DSV, RenderGraphLoadOp_e::LOAD) // Reads only
	.SetExecuteCallback([=](RenderGraph_s& RG, GPUContext_s& Ctx)
	{
		Ctx.SetRootSignature(SpaceRenderer_c::GetRootSignature());

		rl::RenderTargetView_t RTV = RG.GetRTV(Target);
		Ctx.SetRenderTargets(&RTV, 1, RG.GetDSV(SceneDepth)); // TODO: this should be set by the graph

		rl::Viewport vp{ ScreenSize.x, ScreenSize.y };
		Ctx.SetViewports(&vp, 1);
		Ctx.SetDefaultScissor();

		Ctx.SetGraphicsRootCBV(SpaceRendererRootSigSlots::RS_VIEW_BUF, ViewUniforms);

		for (uint32_t StreamIt = 0; StreamIt < NumStreams; ++StreamIt)
		{
			const DebugDrawStream_s& Stream = Streams[StreamIt];
			Ctx.SetPipelineState(Stream.PSO);
			Ctx.SetGraphicsRootSRV(SpaceRendererRootSigSlots::RS_DRAWCONSTANTS, Stream.Vertices);
			Ctx.DrawInstanced(Stream.VertexCount, 1u, 0u, 0u);
		}
	});
}
