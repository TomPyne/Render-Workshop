#pragma once

#include <Render/Render.h>
#include <Shared/RenderUtils/RenderGraph/RenderGraph.h>

struct GBuffer_s
{
	RenderGraphResourceHandle_t Color;
	RenderGraphResourceHandle_t Normal;
	RenderGraphResourceHandle_t RoughnessMetallic;
	RenderGraphResourceHandle_t Velocity;
	RenderGraphResourceHandle_t Depth;
};

class GameRenderer_c
{
public:

	virtual bool Init();
	virtual void Render(rl::RenderView* View, rl::CommandListSubmissionGroup* CLGroup);

	virtual void RenderOpaque(RenderGraphBuilder_s& RGBuilder, const GBuffer_s& GBuffer) {}
};