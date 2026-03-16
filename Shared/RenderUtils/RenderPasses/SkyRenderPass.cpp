#include "SkyRenderPass.h"

#include "Logging/Logging.h"
#include "ModelUtils/SphereBuilder.h"

#include <Render/Render.h>

struct SkyRenderPassUniforms
{
	matrix ViewProjection;
};

void SkyRenderer_s::Init()
{
	{
		SphereBuilder::SphereMeshDesc_s Desc = {};
		Desc.LatitudeSegments = 16;
		Desc.LongitudeSegments = 32;
		Desc.Radius = -1.0f;
		SphereBuilder::SphereMesh_s SkySphereMesh = SphereBuilder::BuildSphereMesh(Desc);

		if(!ENSUREMSG(SkySphereMesh.Positions.empty(), "[SkyRenderer] Failed to generate a valid sky sphere mesh"))
		{
			return;
		}

		SkySphereVertexBuffer = rl::CreateVertexBufferFromArray(SkySphereMesh.Positions.data(), SkySphereMesh.Positions.size());
		SkySphereIndexBuffer = rl::CreateIndexBufferFromArray(SkySphereMesh.Indices.data(), SkySphereMesh.Indices.size());
		NumIndices = static_cast<uint32_t>(SkySphereMesh.Indices.size());

		if (!ENSUREMSG(SkySphereVertexBuffer && SkySphereIndexBuffer, "[SkyRenderer] Failed to generate render buffers"))
		{
			return;
		}
	}

	{
		rl::GraphicsPipelineStateDesc PSODesc = {};
		PSODesc.RasterizerDesc(rl::PrimitiveTopologyType::TRIANGLE, rl::FillMode::SOLID, rl::CullMode::BACK)
			.DepthDesc(false)
			.TargetBlendDesc({ rl::RenderFormat::R16G16B16A16_FLOAT }, { rl::BlendMode::None() }, rl::RenderFormat::UNKNOWN)
			.VertexShader(rl::CreateVertexShader("Shared/Shaders/Atmosphere/SkySphere.hlsl"))
			.PixelShader(rl::CreatePixelShader("Shared/Shaders/Atmosphere/SkySphere.hlsl"));

		rl::InputElementDesc InputLayout[] =
		{
			{ "POSITION", 0, rl::RenderFormat::R32G32B32_FLOAT, 0, 0, rl::InputClassification::PER_VERTEX, 0 },
		};
		SkyPSO = rl::CreateGraphicsPipelineState(PSODesc, InputLayout, 1u);

		if (!ENSUREMSG(SkyPSO, "[SkyRenderer] Failed to generate PSO"))
		{
			return;
		}

		Ready = true;
	}
}

void SkyRenderer_s::AddPass(RenderGraphBuilder_s& RGBuilder, RenderGraphResourceHandle_t SceneColorTarget, const matrix& ViewProjection, uint32_t CBVSlot)
{
	if (!Ready)
		return;

	RenderGraphPass_s& MeshDrawPass = RGBuilder.AddPass(RenderGraphPassType_e::GRAPHICS, L"Mesh Pass")
	.AccessResource(SceneColorTarget, RenderGraphResourceAccessType_e::RTV, RenderGraphLoadOp_e::LOAD)
	.SetExecuteCallback([=, this](RenderGraph_s& RG, rl::CommandList* CL)
	{
		CL->SetRootSignature();

		rl::RenderTargetView_t SceneColorRTV = RG.GetRTV(SceneColorTarget);

		uint2 SceneColorDim = RG.GetTextureDimensions(SceneColorTarget);

		CL->SetRenderTargets(&SceneColorRTV, 1, rl::DepthStencilView_t::INVALID);

		rl::Viewport vp{ SceneColorDim.x, SceneColorDim.y };
		CL->SetViewports(&vp, 1);
		CL->SetDefaultScissor(); // Could also be captured by the command context

		SkyRenderPassUniforms UniformData = {}; // Capture uniforms outside of the lambda.
		UniformData.ViewProjection = ViewProjection;

		CL->SetGraphicsRootCBV(CBVSlot, rl::CreateDynamicConstantBuffer(&UniformData));

		CL->SetPipelineState(SkyPSO);

		CL->SetVertexBuffer(0, SkySphereVertexBuffer, static_cast<uint32_t>(sizeof(float3)), 0u);
		CL->SetIndexBuffer(SkySphereIndexBuffer, rl::RenderFormat::R16_UINT, 0u);
		CL->DrawIndexedInstanced(NumIndices, 1u, 0u, 0u, 0u);
	});
}
