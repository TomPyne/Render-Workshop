#include <Render/Render.h>
#include <SurfMath.h>
#include "imgui.h"

#include <Camera/FlyCamera.h>
#include <Logging/Logging.h>
#include <ModelUtils/Model.h>
#include <ModelUtils/ModelLoader.h>

using namespace tpr;

enum RootSigSlots
{
	RS_VIEW_BUF,
	RS_MAT_BUF,
	RS_SRV_TABLE,
	RS_UAV_TABLE,
	RS_COUNT,
};

struct Globals_s
{
	Model_s Model;

	// Targets
	uint32_t ScreenWidth = 0;
	uint32_t ScreenHeight = 0;

	TexturePtr ColorTexture = {};
	RenderTargetViewPtr ColorRTV = {};
	ShaderResourceViewPtr ColorSRV = {};

	TexturePtr NormalTexture = {};
	RenderTargetViewPtr NormalRTV = {};
	ShaderResourceViewPtr NormalSRV = {};

	TexturePtr DepthTexture = {};
	DepthStencilViewPtr DepthDSV = {};

	// Camera
	FlyCamera Cam;

	// Shaders
	GraphicsPipelineStatePtr MeshPSO;
	GraphicsPipelineStatePtr DeferredPSO;
} G;

tpr::RenderInitParams GetAppRenderParams()
{
	tpr::RenderInitParams Params;
#ifdef _DEBUG
	Params.DebugEnabled = true;
#else
	Params.DebugEnabled = false;
#endif

	Params.RootSigDesc.Flags = RootSignatureFlags::ALLOW_INPUT_LAYOUT;
	Params.RootSigDesc.Slots.resize(RS_COUNT);
	Params.RootSigDesc.Slots[RS_VIEW_BUF] = RootSignatureSlot::CBVSlot(0, 0);
	Params.RootSigDesc.Slots[RS_MAT_BUF] = RootSignatureSlot::CBVSlot(1, 0);
	Params.RootSigDesc.Slots[RS_SRV_TABLE] = RootSignatureSlot::DescriptorTableSlot(0, 0, tpr::RootSignatureDescriptorTableType::SRV);
	Params.RootSigDesc.Slots[RS_UAV_TABLE] = RootSignatureSlot::DescriptorTableSlot(0, 0, tpr::RootSignatureDescriptorTableType::UAV);

	Params.RootSigDesc.GlobalSamplers.resize(2);
	Params.RootSigDesc.GlobalSamplers[0].AddressModeUVW(SamplerAddressMode::WRAP).FilterModeMinMagMip(SamplerFilterMode::LINEAR);
	Params.RootSigDesc.GlobalSamplers[1].AddressModeUVW(SamplerAddressMode::CLAMP).FilterModeMinMagMip(SamplerFilterMode::LINEAR);

	return Params;
}

bool InitializeApp()
{
	if (!ENSUREMSG(Render_IsBindless(), "Only Bindless renderer supported for app"))
	{
		return false;
	}

	if (!LoadModelFromWavefront(L"Assets/Rungholt.obj", G.Model))
	{
		LOGERROR("Failed to load model");
		return false;
	}

	// Mesh PSO
	{
		VertexShader_t MeshVS = CreateVertexShader("Shaders/Mesh.hlsl");
		PixelShader_t MeshPS = CreatePixelShader("Shaders/Mesh.hlsl");

		GraphicsPipelineStateDesc PsoDesc = {};
		PsoDesc.RasterizerDesc(PrimitiveTopologyType::TRIANGLE, FillMode::SOLID, CullMode::BACK)
			.DepthDesc(true, ComparisionFunc::LESS_EQUAL)
			.TargetBlendDesc({ RenderFormat::R16G16B16A16_FLOAT, RenderFormat::R16G16B16A16_FLOAT }, { BlendMode::None(), BlendMode::None()}, RenderFormat::D16_UNORM)
			.VertexShader(MeshVS)
			.PixelShader(MeshPS);

		InputElementDesc InputDesc[] =
		{
			{"POSITION", 0, RenderFormat::R32G32B32_FLOAT, 0, 0, InputClassification::PER_VERTEX, 0 },
			{"TEXCOORD", 0, RenderFormat::R32G32_FLOAT, 1, 0, InputClassification::PER_VERTEX, 0 },
			{"NORMAL", 0, RenderFormat::R32G32B32_FLOAT, 2, 0, InputClassification::PER_VERTEX, 0 },
		};

		G.MeshPSO = CreateGraphicsPipelineState(PsoDesc, InputDesc, ARRAYSIZE(InputDesc));
	}

	// Deferred PSO
	{
		VertexShader_t DeferredVS = CreateVertexShader("Shaders/Deferred.hlsl");
		PixelShader_t DeferredPS = CreatePixelShader("Shaders/Deferred.hlsl");

		GraphicsPipelineStateDesc PsoDesc = {};
		PsoDesc.RasterizerDesc(PrimitiveTopologyType::TRIANGLE, FillMode::SOLID, CullMode::BACK)
			.DepthDesc(false)
			.TargetBlendDesc({ RenderFormat::R8G8B8A8_UNORM }, { BlendMode::None() }, RenderFormat::UNKNOWN)
			.VertexShader(DeferredVS)
			.PixelShader(DeferredPS);

		G.DeferredPSO = CreateGraphicsPipelineState(PsoDesc);
	}

	G.Cam.SetPosition(float3(-5, 20, 25));
	G.Cam.SetNearFar(0.1f, 1000.0f);

	return true;
}

void ResizeApp(uint32_t width, uint32_t height)
{
	width = Max(width, 1u);
	height = Max(height, 1u);

	if (G.ScreenWidth == width && G.ScreenHeight == height)
	{
		return;
	}

	G.ScreenWidth = width;
	G.ScreenHeight = height;

	TextureCreateDescEx ColorDesc = {};
	ColorDesc.DebugName = L"SceneColor";
	ColorDesc.Flags = RenderResourceFlags::RTV | RenderResourceFlags::SRV;
	ColorDesc.ResourceFormat = RenderFormat::R16G16B16A16_FLOAT;
	ColorDesc.Height = G.ScreenHeight;
	ColorDesc.Width = G.ScreenWidth;
	ColorDesc.InitialState = ResourceTransitionState::RENDER_TARGET;
	ColorDesc.Dimension = TextureDimension::TEX2D;
	G.ColorTexture = CreateTextureEx(ColorDesc);
	G.ColorRTV = CreateTextureRTV(G.ColorTexture, RenderFormat::R16G16B16A16_FLOAT, TextureDimension::TEX2D, 1u);
	G.ColorSRV = CreateTextureSRV(G.ColorTexture, RenderFormat::R16G16B16A16_FLOAT, TextureDimension::TEX2D, 1u, 1u);

	TextureCreateDescEx NormalDesc = {};
	NormalDesc.DebugName = L"SceneNormal";
	NormalDesc.Flags = RenderResourceFlags::RTV | RenderResourceFlags::SRV;
	NormalDesc.ResourceFormat = RenderFormat::R16G16B16A16_FLOAT;
	NormalDesc.Height = G.ScreenHeight;
	NormalDesc.Width = G.ScreenWidth;
	NormalDesc.InitialState = ResourceTransitionState::RENDER_TARGET;
	NormalDesc.Dimension = TextureDimension::TEX2D;
	G.NormalTexture = CreateTextureEx(NormalDesc);
	G.NormalRTV = CreateTextureRTV(G.NormalTexture, RenderFormat::R16G16B16A16_FLOAT, TextureDimension::TEX2D, 1u);
	G.NormalSRV = CreateTextureSRV(G.NormalTexture, RenderFormat::R16G16B16A16_FLOAT, TextureDimension::TEX2D, 1u, 1u);

	TextureCreateDescEx DepthDesc = {};
	DepthDesc.DebugName = L"SceneDepth";
	DepthDesc.Flags = RenderResourceFlags::DSV;
	DepthDesc.ResourceFormat = RenderFormat::D16_UNORM;
	DepthDesc.Height = G.ScreenHeight;
	DepthDesc.Width = G.ScreenWidth;
	DepthDesc.InitialState = ResourceTransitionState::DEPTH_WRITE;
	DepthDesc.Dimension = TextureDimension::TEX2D;
	G.DepthTexture = CreateTextureEx(DepthDesc);
	G.DepthDSV = CreateTextureDSV(G.DepthTexture, RenderFormat::D16_UNORM, TextureDimension::TEX2D, 1u);

	G.Cam.Resize(G.ScreenWidth, G.ScreenHeight);
}

void Update(float deltaSeconds)
{
	G.Cam.UpdateView(deltaSeconds);
}

void ImguiUpdate()
{
	ImGui::ShowDemoWindow();
}

void Render(tpr::RenderView* view, tpr::CommandListSubmissionGroup* clGroup, float deltaSeconds)
{
	struct
	{
		matrix viewProjection;
		float3 CamPos;
		float __pad;
	} ViewConsts;

	ViewConsts.viewProjection = G.Cam.GetView() * G.Cam.GetProjection();
	ViewConsts.CamPos = G.Cam.GetPosition();

	DynamicBuffer_t ViewCBuf = CreateDynamicConstantBuffer(&ViewConsts);

	struct
	{
		uint32_t SceneColorTextureIndex;
		uint32_t SceneNormalTextureIndex;
	} DeferredConsts;

	DeferredConsts.SceneColorTextureIndex = GetDescriptorIndex(G.ColorSRV);
	DeferredConsts.SceneNormalTextureIndex = GetDescriptorIndex(G.NormalSRV);

	DynamicBuffer_t DeferredCBuf = CreateDynamicConstantBuffer(&DeferredConsts);

	CommandList* cl = clGroup->CreateCommandList();

	cl->SetRootSignature();

	// Bind and clear targets
	{
		constexpr float DefaultClearCol[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		cl->ClearRenderTarget(G.ColorRTV, DefaultClearCol);
		cl->ClearRenderTarget(G.NormalRTV, DefaultClearCol);
		cl->ClearDepth(G.DepthDSV, 1.0f);

		RenderTargetView_t SceneTargets[] = { G.ColorRTV, G.NormalRTV };
		cl->SetRenderTargets(SceneTargets, ARRAYSIZE(SceneTargets), G.DepthDSV);
	}

	// Init viewport and scissor
	{
		Viewport vp{ G.ScreenWidth, G.ScreenHeight };
		cl->SetViewports(&vp, 1);
		cl->SetDefaultScissor();
	}

	// Bind draw buffers
	{
		cl->SetGraphicsRootCBV(0, ViewCBuf);
		cl->SetGraphicsRootDescriptorTable(RS_SRV_TABLE);
	}

	cl->SetPipelineState(G.MeshPSO);
	for (const Mesh_s& Mesh : G.Model.Meshes)
	{
		cl->SetGraphicsRootCBV(RS_MAT_BUF, Mesh.Material.MaterialBuffer);

		cl->SetVertexBuffers(0u, Mesh.BoundBufferCount, Mesh.BindBuffers, Mesh.BindStrides, Mesh.BindOffsets);
		cl->SetIndexBuffer(Mesh.Index.Buffer, Mesh.Index.Format, Mesh.Index.Offset);

		cl->DrawIndexedInstanced(Mesh.Index.Count, 1u, 0u, 0u, 0u);
	}

	// Transition for deferred pass
	{
		cl->TransitionResource(G.ColorTexture, tpr::ResourceTransitionState::RENDER_TARGET, tpr::ResourceTransitionState::PIXEL_SHADER_RESOURCE);
		cl->TransitionResource(G.NormalTexture, tpr::ResourceTransitionState::RENDER_TARGET, tpr::ResourceTransitionState::PIXEL_SHADER_RESOURCE);
	}

	// Bind back buffer target
	{
		constexpr float DefaultClearCol[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

		RenderTargetView_t backBufferRtv = view->GetCurrentBackBufferRTV();

		cl->ClearRenderTarget(backBufferRtv, DefaultClearCol);

		cl->SetRenderTargets(&backBufferRtv, 1, G.DepthDSV);
	}

	// Render deferred
	{
		cl->SetGraphicsRootCBV(0, DeferredCBuf);

		cl->SetPipelineState(G.DeferredPSO);

		cl->DrawInstanced(6u, 1u, 0u, 0u);
	}

	// Transition for next pass
	{
		cl->TransitionResource(G.ColorTexture, tpr::ResourceTransitionState::PIXEL_SHADER_RESOURCE, tpr::ResourceTransitionState::RENDER_TARGET);
		cl->TransitionResource(G.NormalTexture, tpr::ResourceTransitionState::PIXEL_SHADER_RESOURCE, tpr::ResourceTransitionState::RENDER_TARGET);
	}
}

void ShutdownApp()
{

}
