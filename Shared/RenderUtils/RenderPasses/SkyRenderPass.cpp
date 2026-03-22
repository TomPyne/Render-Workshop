#include "SkyRenderPass.h"

#include "Logging/Logging.h"
#include "ModelUtils/SphereBuilder.h"

#include <Render/Render.h>

struct SkyRenderPassUniforms
{
	matrix ViewProjection;

	float3 CamPos;
	float PlanetRadius;

	float AtmosphereRadius;
	float3 SunDirection;

	float AtmosphereThicknessR;
	float AtmosphereThicknessM;
	float2 __Pad;
};

void SkyRenderer_s::Init(uint32_t InCBVSlot)
{
	{
		SphereBuilder::SphereMeshDesc_s Desc = {};
		Desc.LatitudeSegments = 16;
		Desc.LongitudeSegments = 32;
		Desc.Radius = -1.0f;
		SphereBuilder::SphereMesh_s SkySphereMesh = SphereBuilder::BuildSphereMesh(Desc);

		if(!ENSUREMSG(!SkySphereMesh.Positions.empty(), "[SkyRenderer] Failed to generate a valid sky sphere mesh"))
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
		CBVSlot = InCBVSlot;

		std::string CBVSlotDef = "b" + std::to_string(CBVSlot);
		rl::ShaderMacros Macros = { { "CBV_SLOT", CBVSlotDef.c_str() } };

		rl::GraphicsPipelineStateDesc PSODesc = {};
		PSODesc.RasterizerDesc(rl::PrimitiveTopologyType::TRIANGLE, rl::FillMode::SOLID, rl::CullMode::BACK)
			.DepthDesc(false, rl::ComparisionFunc::LESS_EQUAL)
			.TargetBlendDesc({ rl::RenderFormat::R16G16B16A16_FLOAT }, { rl::BlendMode::None() }, rl::RenderFormat::D32_FLOAT)
			.VertexShader(rl::CreateVertexShader("Shared/Shaders/Atmosphere/SkySphere.hlsl", Macros))
			.PixelShader(rl::CreatePixelShader("Shared/Shaders/Atmosphere/SkySphere.hlsl", Macros));

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

void SkyRenderer_s::AddPass(RenderGraphBuilder_s& RGBuilder, RenderGraphResourceHandle_t SceneColorTarget, RenderGraphResourceHandle_t SceneDepth, const matrix& ViewProjection, const float3& CamPos, const float3& SunDirection)
{
	if (!Ready)
		return;

	RenderGraphPass_s& MeshDrawPass = RGBuilder.AddPass(RenderGraphPassType_e::GRAPHICS, L"Mesh Pass")
	.AccessResource(SceneColorTarget, RenderGraphResourceAccessType_e::RTV, RenderGraphLoadOp_e::CLEAR)
	.AccessResource(SceneDepth, RenderGraphResourceAccessType_e::DSV, RenderGraphLoadOp_e::CLEAR)
	.SetExecuteCallback([=, this](RenderGraph_s& RG, rl::CommandList* CL)
	{
		CL->SetRootSignature();

		rl::RenderTargetView_t SceneColorRTV = RG.GetRTV(SceneColorTarget);
		rl::DepthStencilView_t SceneDepthDSV = RG.GetDSV(SceneDepth);

		uint2 SceneColorDim = RG.GetTextureDimensions(SceneColorTarget);

		CL->SetRenderTargets(&SceneColorRTV, 1, SceneDepthDSV);

		rl::Viewport vp{ SceneColorDim.x, SceneColorDim.y };
		CL->SetViewports(&vp, 1);
		CL->SetDefaultScissor(); // Could also be captured by the command context

		SkyRenderPassUniforms UniformData = {}; // Capture uniforms outside of the lambda.
		UniformData.ViewProjection = ViewProjection;
		UniformData.CamPos = CamPos;
		UniformData.PlanetRadius = 6360e3f;
		UniformData.AtmosphereRadius = 6420e3;
		UniformData.SunDirection = SunDirection;
		UniformData.AtmosphereThicknessR = 7994.0f;
		UniformData.AtmosphereThicknessM = 1200.0f;

		CL->SetGraphicsRootCBV(CBVSlot, rl::CreateDynamicConstantBuffer(&UniformData));

		CL->SetPipelineState(SkyPSO);

		CL->SetVertexBuffer(0, SkySphereVertexBuffer, static_cast<uint32_t>(sizeof(float3)), 0u);
		CL->SetIndexBuffer(SkySphereIndexBuffer, rl::RenderFormat::R16_UINT, 0u);
		CL->DrawIndexedInstanced(NumIndices, 1u, 0u, 0u, 0u);
	});
}
