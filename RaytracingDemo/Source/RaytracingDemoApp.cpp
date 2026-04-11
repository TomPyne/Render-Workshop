#include <Render/Render.h>
#include <Render/Raytracing.h>
#include "RTDGlobals.h"
#include "RTDModel.h"
#include <SurfMath.h>
#include "imgui.h"

#include <Assets/Assets.h>
#include <Camera/FlyCamera.h>
#include <FileUtils/FileStream.h>
#include <Logging/Logging.h>
#include <RenderUtils/GPUContext/GPUContext.h>
#include <RenderUtils/RenderGraph/RenderGraph.h>
#include <RenderUtils/RenderPasses/DisocclusionRenderPass.h>
#include <RenderUtils/RenderPasses/SkyRenderPass.h>
#include <RenderUtils/RenderPasses/ScreenTracedAmbientOcclusion.h>
#include <RenderUtils/RenderPasses/ScreenTracedReflections.h>

#include <HPModel.h>
#include <HPWfMtlLib.h>
#include <HPTexture.h>

#include <ppl.h>

using namespace rl;

namespace GlobalRootSigSlots
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

namespace RTRootSigSlots
{
	enum Value
	{
		RS_CONSTANTS,
		RS_RAYTRACING_SCENE,
		RS_SRV_TABLE,
		RS_UAV_TABLE,
		RS_COUNT,
	};
}

struct Globals_s
{
	std::vector<RTDModel_s> Models;

	// Targets
	uint32_t ScreenWidth = 0;
	uint32_t ScreenHeight = 0;
	
	RenderGraphTexturePtr_t SceneConfidenceHistoryRGTexture = {};
	RenderGraphTexturePtr_t SceneShadowHistoryRGTexture = {};
	RenderGraphTexturePtr_t SceneDepthHistoryRGTexture = {};

	// Camera
	FlyCamera Cam;

	matrix PrevViewProjection;
	uint32_t FramesSinceMove = 0;

	// Shaders
	GraphicsPipelineStatePtr MeshVSPSO;
	GraphicsPipelineStatePtr MeshMSPSO;
	GraphicsPipelineStatePtr DeferredPSO;
	GraphicsPipelineStatePtr DebugViewPSO;
	GraphicsPipelineStatePtr U2TonemapPSO;
	GraphicsPipelineStatePtr NoTonemapPSO;
	ComputePipelineStatePtr UAVClearF1PSO;
	ComputePipelineStatePtr UAVClearF2PSO;
	ComputePipelineStatePtr ShadowDenoisePSO;
	ComputePipelineStatePtr ShadowTemporalRecombinePSO;
	RaytracingPipelineStatePtr RTPSO;

	// RT Root Signature
	RootSignaturePtr RTRootSignature;

	// RG
	RenderGraphResourcePool_s RenderGraphResourcePool;

	// Renderers
	SkyRenderer_s SkyRenderer;
	ScreenTracedAmbientOcclusionRenderer_s STAORenderer;
	ScreenTracedReflectionRenderer_s STReflectionRenderer;
	DisocclusionRenderPass_s DisocclusionPass;

	RTDMaterial_s DefaultMaterial = {};

	RaytracingShaderTablePtr RaytracingShaderTable = {};

	bool UseMeshShaders = true;
	bool ShowMeshID = false;
	bool ShowShadows = true;
	int32_t DrawMode = 0;

	float SunYaw = 0.0f;
	float SunPitch = 1.0f;
	float SunSoftAngle = 0.01f;

	float Exposure = 1.0f;
	float WhitePoint = 11.2f;
	float ExposureBias = 2.0f;

	float ElapsedTime = 0.0f;
} G;

static const uint32_t ViewCBVRegister = 1;
static const uint32_t ModelCBVRegister = 2;
static const uint32_t MatCBVRegister = 3;

static const float NearPlaneZ = 0.1f;
static const float FarPlaneZ = 1000.0f;

float3 GetSunDirection()
{
	const float CosTheta = cosf(G.SunPitch);
	const float SinTheta = sinf(G.SunPitch);
	const float CosPhi = cosf(G.SunYaw);
	const float SinPhi = sinf(G.SunYaw);

	return Normalize(float3(CosPhi * CosTheta, SinTheta, SinPhi * CosTheta));
}

rl::RenderInitParams GetAppRenderParams()
{
	rl::RenderInitParams Params;
#ifdef _DEBUG
	Params.DebugEnabled = true;
#else
	Params.DebugEnabled = false;
#endif

	Params.RootSigDesc.Flags = RootSignatureFlags::ALLOW_INPUT_LAYOUT;
	Params.RootSigDesc.Slots.resize(GlobalRootSigSlots::RS_COUNT);
	Params.RootSigDesc.Slots[GlobalRootSigSlots::RS_DRAWCONSTANTS] = RootSignatureSlot::ConstantsSlot(RTDDrawConstantSlots_e::COUNT, 0);
	Params.RootSigDesc.Slots[GlobalRootSigSlots::RS_VIEW_BUF] = RootSignatureSlot::CBVSlot(ViewCBVRegister, 0);
	Params.RootSigDesc.Slots[GlobalRootSigSlots::RS_MODEL_BUF] = RootSignatureSlot::CBVSlot(ModelCBVRegister, 0);
	Params.RootSigDesc.Slots[GlobalRootSigSlots::RS_MAT_BUF] = RootSignatureSlot::CBVSlot(MatCBVRegister, 0);
	Params.RootSigDesc.Slots[GlobalRootSigSlots::RS_SRV_TABLE] = RootSignatureSlot::DescriptorTableSlot(0, 0, rl::RootSignatureDescriptorTableType::SRV);
	Params.RootSigDesc.Slots[GlobalRootSigSlots::RS_UAV_TABLE] = RootSignatureSlot::DescriptorTableSlot(0, 0, rl::RootSignatureDescriptorTableType::UAV);

	Params.RootSigDesc.GlobalSamplers.resize(2);
	Params.RootSigDesc.GlobalSamplers[0].AddressModeUVW(SamplerAddressMode::WRAP).FilterModeMinMagMip(SamplerFilterMode::ANISOTROPIC);
	Params.RootSigDesc.GlobalSamplers[1].AddressModeUVW(SamplerAddressMode::CLAMP).FilterModeMinMagMip(SamplerFilterMode::LINEAR);

	return Params;
}

bool InitializeApp()
{
	GAssetConfig.SkipCookedLoading = true;

	if (!ENSUREMSG(Render_IsBindless(), "Only Bindless renderer supported for app"))
	{
		return false;
	}

	Glob.RaytracingScene = rl::CreateRaytracingScene();

	std::vector<std::wstring> ModelPaths =
	{
		L"Models/bistro2.hp_mdl"
	};

	std::vector<HPModel_s> ModelAssets;
	ModelAssets.resize(ModelPaths.size());

	Concurrency::parallel_for((size_t)0u, ModelPaths.size(), [&](size_t i)
	{
		HPModel_s LoadedModel;
		if (LoadedModel.Serialize(s_AssetDirectory + ModelPaths[i], FileStreamMode_e::READ))
		{
			ModelAssets[i] = std::move(LoadedModel);
		}
	});

	for (const HPModel_s& ModelAsset : ModelAssets)
	{
		G.Models.push_back({});
		G.Models.back().Init(&ModelAsset);
	}

	// Mesh PSO
	{
		VertexShader_t MeshVS = CreateVertexShader("RaytracingDemo/Shaders/Mesh.hlsl");
		MeshShader_t MeshMS = CreateMeshShader("RaytracingDemo/Shaders/Mesh.hlsl");
		PixelShader_t MeshPS = CreatePixelShader("RaytracingDemo/Shaders/Mesh.hlsl");

		GraphicsPipelineStateDesc PsoDesc = {};
		PsoDesc.RasterizerDesc(PrimitiveTopologyType::TRIANGLE, FillMode::SOLID, CullMode::BACK)
			.DepthDesc(true, ComparisionFunc::LESS_EQUAL)
			.TargetBlendDesc({ RenderFormat::R16G16B16A16_FLOAT, RenderFormat::R16G16B16A16_FLOAT, RenderFormat::R16G16_FLOAT, RenderFormat::R16G16_FLOAT }, { BlendMode::None(), BlendMode::None(), BlendMode::None(), BlendMode::None()}, RenderFormat::D32_FLOAT)
			.VertexShader(MeshVS)
			.PixelShader(MeshPS);

		PsoDesc.DebugName = L"MeshVSPSO";
		G.MeshVSPSO = CreateGraphicsPipelineState(PsoDesc);

		PsoDesc.VertexShader(VertexShader_t::INVALID)
			.MeshShader(MeshMS);

		PsoDesc.DebugName = L"MeshMSPSO";
		G.MeshMSPSO = CreateGraphicsPipelineState(PsoDesc);
	}

	// UAV Clear PSO
	{
		ComputePipelineStateDesc PsoDesc = {};

		PsoDesc.Cs = CreateComputeShader("RaytracingDemo/Shaders/ClearUAV.hlsl", { "F1" });
		PsoDesc.DebugName = L"ClearUAVF1";		
		G.UAVClearF1PSO = CreateComputePipelineState(PsoDesc);

		PsoDesc.Cs = CreateComputeShader("RaytracingDemo/Shaders/ClearUAV.hlsl", { "F2" });
		PsoDesc.DebugName = L"ClearUAVF2";
		G.UAVClearF2PSO = CreateComputePipelineState(PsoDesc);
	}

	VertexShader_t ScreenPassVS = CreateVertexShader("RaytracingDemo/Shaders/ScreenPassVS.hlsl");

	// Deferred PSO
	{		
		PixelShader_t DeferredPS = CreatePixelShader("RaytracingDemo/Shaders/Deferred.hlsl");

		GraphicsPipelineStateDesc PsoDesc = {};
		PsoDesc.RasterizerDesc(PrimitiveTopologyType::TRIANGLE, FillMode::SOLID, CullMode::BACK)
			.DepthDesc(false)
			.TargetBlendDesc({ RenderFormat::R8G8B8A8_UNORM }, { BlendMode::None() }, RenderFormat::UNKNOWN)
			.VertexShader(ScreenPassVS)
			.PixelShader(DeferredPS);

		PsoDesc.DebugName = L"DeferredPSO";

		G.DeferredPSO = CreateGraphicsPipelineState(PsoDesc);
	}

	// Debug PSO
	{
		PixelShader_t DebugViewPS = CreatePixelShader("RaytracingDemo/Shaders/DebugView.hlsl");

		GraphicsPipelineStateDesc PsoDesc = {};
		PsoDesc.RasterizerDesc(PrimitiveTopologyType::TRIANGLE, FillMode::SOLID, CullMode::BACK)
			.DepthDesc(false)
			.TargetBlendDesc({ RenderFormat::R8G8B8A8_UNORM }, { BlendMode::None() }, RenderFormat::UNKNOWN)
			.VertexShader(ScreenPassVS)
			.PixelShader(DebugViewPS);

		PsoDesc.DebugName = L"DebugViewPSO";

		G.DebugViewPSO = CreateGraphicsPipelineState(PsoDesc);
	}

	// Tonemapper PSOs
	{
		// No tonemapping
		PixelShader_t NoTonemapPS = CreatePixelShader("RaytracingDemo/Shaders/Tonemapping.hlsl", {"NOTONEMAPPER"});

		GraphicsPipelineStateDesc PsoDesc = {};

		PsoDesc.RasterizerDesc(PrimitiveTopologyType::TRIANGLE, FillMode::SOLID, CullMode::BACK)
			.DepthDesc(false)
			.TargetBlendDesc({ RenderFormat::R8G8B8A8_UNORM }, { BlendMode::None() }, RenderFormat::UNKNOWN)
			.VertexShader(ScreenPassVS)
			.PixelShader(NoTonemapPS);
		PsoDesc.DebugName = L"NoTonemapPSO";
		G.NoTonemapPSO = CreateGraphicsPipelineState(PsoDesc);

		PixelShader_t U2TonemapPS = CreatePixelShader("RaytracingDemo/Shaders/Tonemapping.hlsl", { "U2TONEMAPPER" });
		PsoDesc.PixelShader(U2TonemapPS);
		PsoDesc.DebugName = L"U2TonemapPSO";
		G.U2TonemapPSO = CreateGraphicsPipelineState(PsoDesc);
	}

	// Denoiser PSO
	{
		ComputeShader_t ShadowDenoiserCS = CreateComputeShader("RaytracingDemo/Shaders/ShadowDenoiser.hlsl");
		ComputePipelineStateDesc PsoDesc = {};
		PsoDesc.Cs = ShadowDenoiserCS;
		PsoDesc.DebugName = L"ShadowDenoiser";

		G.ShadowDenoisePSO = CreateComputePipelineState(PsoDesc);
	}

	// Shadow Temporal Recombine PSO
	{
		ComputeShader_t ShadowTemporalRecombineCS = CreateComputeShader("RaytracingDemo/Shaders/ShadowTemporalRecombine.hlsl");
		ComputePipelineStateDesc PsoDesc = {};
		PsoDesc.Cs = ShadowTemporalRecombineCS;
		PsoDesc.DebugName = L"ShadowTemporalRecombine";
		G.ShadowTemporalRecombinePSO = CreateComputePipelineState(PsoDesc);
	}

	// RT PSO
	{
		RootSignatureDesc RTRootSignatureDesc = {};
		RTRootSignatureDesc.Slots.resize(RTRootSigSlots::RS_COUNT);
		RTRootSignatureDesc.Slots[RTRootSigSlots::RS_CONSTANTS] = RootSignatureSlot::CBVSlot(0, 0);
		RTRootSignatureDesc.Slots[RTRootSigSlots::RS_RAYTRACING_SCENE] = RootSignatureSlot::SRVSlot(0, 0);
		RTRootSignatureDesc.Slots[RTRootSigSlots::RS_SRV_TABLE] = RootSignatureSlot::DescriptorTableSlot(1, 0, rl::RootSignatureDescriptorTableType::SRV);
		RTRootSignatureDesc.Slots[RTRootSigSlots::RS_UAV_TABLE] = RootSignatureSlot::DescriptorTableSlot(0, 0, rl::RootSignatureDescriptorTableType::UAV);

		G.RTRootSignature = CreateRootSignature(RTRootSignatureDesc);

		rl::RaytracingPipelineStateDesc RTDesc = {};
		RTDesc.RayGenShader = rl::CreateRayGenShader("RaytracingDemo/Shaders/RTShadows.hlsl");
		RTDesc.MissShader = rl::CreateMissShader("RaytracingDemo/Shaders/RTShadows.hlsl");
		RTDesc.DebugName = L"RTShadow";
		RTDesc.RootSig = G.RTRootSignature;
		G.RTPSO = CreateRaytracingPipelineState(RTDesc);

		BuildRaytracingScene(Glob.RaytracingScene);

		RaytracingShaderTableLayout ShaderTableLayout;
		ShaderTableLayout.RayGenShader = RTDesc.RayGenShader;
		ShaderTableLayout.MissShader = RTDesc.MissShader;
		G.RaytracingShaderTable = CreateRaytracingShaderTable(G.RTPSO, ShaderTableLayout);
	}

	G.SkyRenderer.Init(GlobalRootSigSlots::RS_VIEW_BUF, ViewCBVRegister);
	G.STAORenderer.Init(GlobalRootSigSlots::RS_UAV_TABLE, GlobalRootSigSlots::RS_SRV_TABLE, GlobalRootSigSlots::RS_VIEW_BUF, ViewCBVRegister);
	G.STReflectionRenderer.Init(GlobalRootSigSlots::RS_UAV_TABLE, GlobalRootSigSlots::RS_SRV_TABLE, GlobalRootSigSlots::RS_VIEW_BUF, ViewCBVRegister);
	G.DisocclusionPass.Init(GlobalRootSigSlots::RS_UAV_TABLE, GlobalRootSigSlots::RS_SRV_TABLE, GlobalRootSigSlots::RS_VIEW_BUF, ViewCBVRegister);

	// Create default material
	G.DefaultMaterial.MaterialConstantBuffer = rl::CreateConstantBuffer(&G.DefaultMaterial.Params);

	G.Cam.SetPosition(float3(-5, 20, 25));
	G.Cam.SetNearFar(NearPlaneZ, FarPlaneZ);

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

	G.SceneConfidenceHistoryRGTexture = CreateRenderGraphTexture(G.ScreenWidth, G.ScreenHeight, RenderFormat::R8_UNORM, RenderGraphResourceAccessType_e::UAV | RenderGraphResourceAccessType_e::SRV, L"SceneConfidenceHistory");
	G.SceneShadowHistoryRGTexture = CreateRenderGraphTexture(G.ScreenWidth, G.ScreenHeight, RenderFormat::R8_UNORM, RenderGraphResourceAccessType_e::UAV | RenderGraphResourceAccessType_e::SRV, L"SceneShadowHistory");
	G.SceneDepthHistoryRGTexture = CreateRenderGraphTexture(G.ScreenWidth, G.ScreenHeight, RenderFormat::R32_FLOAT, RenderGraphResourceAccessType_e::UAV | RenderGraphResourceAccessType_e::SRV, L"SceneDepthHistory");

	G.DisocclusionPass.Resize(G.ScreenWidth, G.ScreenHeight);
	G.STAORenderer.Resize(G.ScreenWidth, G.ScreenHeight);

	G.Cam.Resize(G.ScreenWidth, G.ScreenHeight);
}

void Update(float deltaSeconds)
{
	G.ElapsedTime += deltaSeconds;

	G.Cam.UpdateView(deltaSeconds);
}

void ImguiUpdate()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("Modules"))
		{
			ImGui::MenuItem("ST AO", "", &G.STAORenderer.MenuOpen);
			ImGui::MenuItem("ST Reflections", "", &G.STReflectionRenderer.MenuOpen);
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	G.STAORenderer.DrawImGuiMenu();
	G.STReflectionRenderer.DrawImGuiMenu();

	if (ImGui::Begin("RaytracingDemo"))
	{
		ImGui::Checkbox("Use Mesh Shaders", &G.UseMeshShaders);
		ImGui::Checkbox("Show Mesh ID", &G.ShowMeshID);
		ImGui::Checkbox("Show Shadows", &G.ShowShadows);
		const char* DrawModeNames = "Lit\0Color\0Normal\0Roughness\0Metallic\0Depth\0Position\0Lighting\0RTShadows\0Velocity\0Disocclusion\0AO\0";
		ImGui::Combo("Draw Mode", &G.DrawMode, DrawModeNames);
		ImGui::Separator();
		if (ImGui::SliderAngle("Sun Yaw", &G.SunYaw, 0.0f, 360.0f))
		{
			G.FramesSinceMove = 0;
		}
		if (ImGui::SliderAngle("Sun Pitch", &G.SunPitch, -90.0f, 90.0f))
		{
			G.FramesSinceMove = 0;
		}
		if (ImGui::SliderAngle("Sun Soft Angle", &G.SunSoftAngle, 0.0f, 5.0f))
		{
			G.FramesSinceMove = 0;
		}
		ImGui::Separator();
		ImGui::InputFloat("Exposure", &G.Exposure);
		ImGui::InputFloat("White Point", &G.WhitePoint);
		ImGui::InputFloat("Exposure Bias", &G.ExposureBias);
		ImGui::Separator();
		if (ImGui::Button("Recompile Shaders"))
		{
			ReloadShaders();
			ReloadPipelines();
		}
	}
	ImGui::End();
}

static bool WantsTonemap()
{
	return G.DrawMode == 0 || G.DrawMode == 7; // Lit or Lighting
}

static void FullScreenPassVSPS(RenderGraph_s& RG, GPUContext_s& Ctx, RenderGraphResourceHandle_t Target, rl::GraphicsPipelineState_t PSO, DynamicBuffer_t UniformBuffer)
{
	Ctx.SetRootSignature();

	rl::RenderTargetView_t BackBufferRTV = RG.GetRTV(Target);

	Ctx.SetRenderTargets(&BackBufferRTV, 1, {}); // TODO: this should be set by the graph

	Viewport vp{ G.ScreenWidth, G.ScreenHeight };
	Ctx.SetViewports(&vp, 1);
	Ctx.SetDefaultScissor(); // Could also be captured by the command context

	Ctx.SetGraphicsRootDescriptorTable(GlobalRootSigSlots::RS_SRV_TABLE);
	Ctx.SetGraphicsRootCBV(GlobalRootSigSlots::RS_VIEW_BUF, UniformBuffer);

	Ctx.SetPipelineState(PSO);

	Ctx.DrawInstanced(6u, 1u, 0u, 0u);
}

void Render(rl::RenderView* View, rl::CommandListSubmissionGroup* clGroup, float deltaSeconds)
{
	float3 SunDirection = GetSunDirection();
	matrix ViewProjection = G.Cam.GetView() * G.Cam.GetProjection();

	const bool bCameraMoved = ViewProjection != G.PrevViewProjection;

	if (bCameraMoved)
	{
		G.FramesSinceMove = 0;
	}
	else
	{
		G.FramesSinceMove++;
	}

	RenderGraphBuilder_s RGBuilder(G.RenderGraphResourcePool);

	// Mesh draw pass
	RenderGraphResourceHandle_t SceneColorTexture = RGBuilder.CreateTexture(G.ScreenWidth, G.ScreenHeight, RenderFormat::R16G16B16A16_FLOAT, RenderGraphResourceAccessType_e::RTV | RenderGraphResourceAccessType_e::SRV, L"SceneColorTexture");
	RenderGraphResourceHandle_t SceneNormalTexture = RGBuilder.CreateTexture(G.ScreenWidth, G.ScreenHeight, RenderFormat::R16G16B16A16_FLOAT, RenderGraphResourceAccessType_e::RTV | RenderGraphResourceAccessType_e::SRV, L"SceneNormalTexture");
	RenderGraphResourceHandle_t SceneRoughnessMetallicTexture = RGBuilder.CreateTexture(G.ScreenWidth, G.ScreenHeight, RenderFormat::R16G16_FLOAT, RenderGraphResourceAccessType_e::RTV | RenderGraphResourceAccessType_e::SRV, L"SceneRoughnessMetallicTexture");
	RenderGraphResourceHandle_t SceneVelocityTexture = RGBuilder.CreateTexture(G.ScreenWidth, G.ScreenHeight, RenderFormat::R16G16_FLOAT, RenderGraphResourceAccessType_e::RTV | RenderGraphResourceAccessType_e::SRV, L"SceneVelocityTexture");
	RenderGraphResourceHandle_t SceneDepthTexture = RGBuilder.CreateTexture(G.ScreenWidth, G.ScreenHeight, RenderFormat::R32_FLOAT, RenderGraphResourceAccessType_e::DSV | RenderGraphResourceAccessType_e::SRV, L"SceneDepthTexture");

	G.SkyRenderer.AddPass(RGBuilder, SceneColorTexture, SceneDepthTexture, ViewProjection, G.Cam.GetPosition(), SunDirection);

	RenderGraphPass_s& MeshDrawPass = RGBuilder.AddPass(RenderGraphPassType_e::GRAPHICS, L"Mesh Pass")
	.AccessResource(SceneColorTexture, RenderGraphResourceAccessType_e::RTV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(SceneNormalTexture, RenderGraphResourceAccessType_e::RTV, RenderGraphLoadOp_e::CLEAR)
	.AccessResource(SceneRoughnessMetallicTexture, RenderGraphResourceAccessType_e::RTV, RenderGraphLoadOp_e::CLEAR)
	.AccessResource(SceneVelocityTexture, RenderGraphResourceAccessType_e::RTV, RenderGraphLoadOp_e::CLEAR)
	.AccessResource(SceneDepthTexture, RenderGraphResourceAccessType_e::DSV, RenderGraphLoadOp_e::LOAD)
	.SetExecuteCallback([=](RenderGraph_s& RG, GPUContext_s& Ctx)
	{
		struct
		{
			matrix ViewProjection;
			matrix PrevviewProjection;
			float3 CamPos;
			uint32_t DebugMeshID;
			float2 ScreenSizeRcp;
			float __Pad[2];
		} ViewConsts;

		ViewConsts.ViewProjection = ViewProjection;
		ViewConsts.PrevviewProjection = G.PrevViewProjection;
		ViewConsts.CamPos = G.Cam.GetPosition();
		ViewConsts.DebugMeshID = G.ShowMeshID;
		ViewConsts.ScreenSizeRcp = float2(1.0f / G.ScreenWidth, 1.0f / G.ScreenHeight);

		DynamicBuffer_t ViewCBuf = CreateDynamicConstantBuffer(&ViewConsts);

		Ctx.SetRootSignature();

		rl::RenderTargetView_t SceneRTVs[] = 
		{ 
			RG.GetRTV(SceneColorTexture),
			RG.GetRTV(SceneNormalTexture),
			RG.GetRTV(SceneRoughnessMetallicTexture),
			RG.GetRTV(SceneVelocityTexture)
		};
		rl::DepthStencilView_t SceneDSV = RG.GetDSV(SceneDepthTexture);
		Ctx.SetRenderTargets(SceneRTVs, ARRAYSIZE(SceneRTVs), SceneDSV); // TODO: this should be set by the graph

		Viewport vp{ G.ScreenWidth, G.ScreenHeight };
		Ctx.SetViewports(&vp, 1);
		Ctx.SetDefaultScissor(); // Could also be captured by the command context

		Ctx.SetGraphicsRootCBV(GlobalRootSigSlots::RS_VIEW_BUF, ViewCBuf);
		Ctx.SetGraphicsRootDescriptorTable(GlobalRootSigSlots::RS_SRV_TABLE); // Root sig stuff is trickier

		Ctx.SetPipelineState(G.UseMeshShaders ? G.MeshMSPSO : G.MeshVSPSO);

		for (const RTDModel_s& Model : G.Models)
		{
			Ctx.SetGraphicsRootCBV(GlobalRootSigSlots::RS_MODEL_BUF, Model.ModelConstantBuffer);

			if (G.UseMeshShaders)
			{
				for (const RTDMesh_s& Mesh : Model.Meshes)
				{
					Ctx.SetGraphicsRootValue(GlobalRootSigSlots::RS_DRAWCONSTANTS, RTDDrawConstantSlots_e::MESHLET_OFFSET, Mesh.MeshletOffset);

					Ctx.SetGraphicsRootCBV(GlobalRootSigSlots::RS_MAT_BUF, Mesh.Material ? Mesh.Material->MaterialConstantBuffer : G.DefaultMaterial.MaterialConstantBuffer);

					Ctx.DispatchMesh(Mesh.MeshletCount, 1u, 1u);
				}
			}
			else
			{
				for (const RTDMesh_s& Mesh : Model.Meshes)
				{
					Ctx.SetGraphicsRootValue(GlobalRootSigSlots::RS_DRAWCONSTANTS, RTDDrawConstantSlots_e::INDEX_OFFSET, Mesh.IndexOffset);

					Ctx.SetGraphicsRootCBV(GlobalRootSigSlots::RS_MAT_BUF, Mesh.Material ? Mesh.Material->MaterialConstantBuffer : G.DefaultMaterial.MaterialConstantBuffer);

					Ctx.DrawInstanced(Mesh.IndexCount, 1u, 0u, 0u);
				}
			}
		}
	});

	// Draw RT shadows or clear RT shadows

	RenderGraphResourceHandle_t DepthHistoryTexture = RGBuilder.InjectTexture(G.SceneDepthHistoryRGTexture, L"PrevFrameDepth");
	RenderGraphResourceHandle_t ShadowTexture = RGBuilder.CreateTexture(G.ScreenWidth, G.ScreenHeight, RenderFormat::R8_UNORM, RenderGraphResourceAccessType_e::UAV | RenderGraphResourceAccessType_e::SRV, L"CurrentFrameShadow");

	RenderGraphResourceHandle_t ConfidenceTexture = G.DisocclusionPass.AddPass(RGBuilder, SceneDepthTexture, DepthHistoryTexture, SceneVelocityTexture, ViewProjection, G.PrevViewProjection, uint2(G.ScreenWidth, G.ScreenHeight));

	if (G.ShowShadows)
	{
		RenderGraphPass_s& RTShadowsPass = RGBuilder.AddPass(RenderGraphPassType_e::RAYTRACING, L"RT Shadows")
		.AccessResource(SceneDepthTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
		.AccessResource(ShadowTexture, RenderGraphResourceAccessType_e::UAV, RenderGraphLoadOp_e::DONT_CARE)
		.SetExecuteCallback([=](RenderGraph_s& RG, GPUContext_s& Ctx)
		{
			struct RayUniforms_s
			{
				matrix CamToWorld;

				float3 SunDirection;
				float SunSoftAngle;

				float2 ScreenResolution;
				uint32_t SceneDepthTextureIndex;
				uint32_t SceneShadowTextureIndex;

				float Time;
				float AccumFrames;
				float __pad[2];
			} RayUniforms;

			RayUniforms.CamToWorld = InverseMatrix(ViewProjection);
			RayUniforms.SunDirection = SunDirection;
			RayUniforms.SunSoftAngle = G.SunSoftAngle;
			RayUniforms.ScreenResolution = float2((float)G.ScreenWidth, (float)G.ScreenHeight);
			RayUniforms.SceneDepthTextureIndex = GetDescriptorIndex(RG.GetSRV(SceneDepthTexture));
			RayUniforms.SceneShadowTextureIndex = GetDescriptorIndex(RG.GetUAV(ShadowTexture));
			RayUniforms.Time = G.ElapsedTime;
			RayUniforms.AccumFrames = (float)G.FramesSinceMove;

			DynamicBuffer_t RayCBuf = CreateDynamicConstantBuffer(&RayUniforms);

			Ctx.SetComputeRootSignature(G.RTRootSignature);

			Ctx.SetComputeRootDescriptorTable(RTRootSigSlots::RS_UAV_TABLE);
			Ctx.SetComputeRootDescriptorTable(RTRootSigSlots::RS_SRV_TABLE);

			Ctx.SetPipelineState(G.RTPSO);

			Ctx.SetComputeRootCBV(RTRootSigSlots::RS_CONSTANTS, RayCBuf);
			Ctx.SetComputeRootSRV(RTRootSigSlots::RS_RAYTRACING_SCENE, Glob.RaytracingScene);

			Ctx.DispatchRays(G.RaytracingShaderTable, G.ScreenWidth, G.ScreenHeight, 1);
		});

		RenderGraphResourceHandle_t ShadowHistoryTexture = RGBuilder.InjectTexture(G.SceneShadowHistoryRGTexture, L"PrevFrameShadow");		

		RenderGraphPass_s& RTShadowTemporalRecombinePass = RGBuilder.AddPass(RenderGraphPassType_e::COMPUTE, L"RT Shadow Temporal Recombine")
		.AccessResource(ShadowTexture, RenderGraphResourceAccessType_e::UAV, RenderGraphLoadOp_e::LOAD)
		.AccessResource(ShadowHistoryTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
		.AccessResource(ConfidenceTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
		.AccessResource(SceneVelocityTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
		.SetExecuteCallback([=](RenderGraph_s& RG, GPUContext_s& Ctx)
		{
			struct TemporalRecombineUniforms_s
			{
				uint32_t ConfidenceTextureIndex;
				uint32_t ShadowTextureIndex;
				uint32_t PrevFrameShadowTextureIndex;
				uint32_t VelocityTextureIndex;

				float2 ViewportSizeRcp;
				uint2 ViewportSize;
					
			} TemporalRecombineUniforms;

			TemporalRecombineUniforms.ConfidenceTextureIndex = RG.GetSRVIndex(ConfidenceTexture);
			TemporalRecombineUniforms.ShadowTextureIndex = RG.GetUAVIndex(ShadowTexture);
			TemporalRecombineUniforms.PrevFrameShadowTextureIndex = RG.GetSRVIndex(ShadowHistoryTexture);
			TemporalRecombineUniforms.VelocityTextureIndex = RG.GetSRVIndex(SceneVelocityTexture);
			TemporalRecombineUniforms.ViewportSizeRcp = float2(1.0f / (float)G.ScreenWidth, 1.0f / (float)G.ScreenHeight);
			TemporalRecombineUniforms.ViewportSize = uint2(G.ScreenWidth, G.ScreenHeight);

			DynamicBuffer_t TemporalRecombineCBuf = CreateDynamicConstantBuffer(&TemporalRecombineUniforms);

			Ctx.SetRootSignature();

			Ctx.RWBarrier(RG.GetTexture(ShadowTexture)); // TODO: this should be handled by the graph

			Ctx.SetComputeRootDescriptorTable(GlobalRootSigSlots::RS_UAV_TABLE);
			Ctx.SetComputeRootDescriptorTable(GlobalRootSigSlots::RS_SRV_TABLE);

			Ctx.SetPipelineState(G.ShadowTemporalRecombinePSO);

			Ctx.SetComputeRootCBV(GlobalRootSigSlots::RS_VIEW_BUF, TemporalRecombineCBuf);

			Ctx.Dispatch(DivideRoundUp(G.ScreenWidth, 8u), DivideRoundUp(G.ScreenHeight, 8u), 1);
		});

		RGBuilder.QueueTextureCopy(ShadowHistoryTexture, ShadowTexture);
		RGBuilder.QueueTextureCopy(DepthHistoryTexture, SceneDepthTexture);		
	}
	else
	{
		RenderGraphPass_s& RTShadowsPass = RGBuilder.AddPass(RenderGraphPassType_e::COMPUTE, L"Clear Shadows")
		.AccessResource(ShadowTexture, RenderGraphResourceAccessType_e::UAV, RenderGraphLoadOp_e::DONT_CARE)
		.SetExecuteCallback([=](RenderGraph_s& RG, GPUContext_s& Ctx)
		{
			struct ClearUniforms_s
			{
				uint32_t UAVIndex;
				uint32_t Width;
				uint32_t Height;
				float __Pad0;

				float ClearVal;
				float3 __Pad1;
			} ClearUniforms;
			ClearUniforms.UAVIndex = GetDescriptorIndex(RG.GetUAV(ShadowTexture));
			ClearUniforms.Width = G.ScreenWidth;
			ClearUniforms.Height = G.ScreenHeight;
			ClearUniforms.ClearVal = 1.0f;
			DynamicBuffer_t ClearBuf = CreateDynamicConstantBuffer(&ClearUniforms);

			Ctx.SetRootSignature();

			Ctx.SetComputeRootDescriptorTable(GlobalRootSigSlots::RS_UAV_TABLE);

			Ctx.SetPipelineState(G.UAVClearF1PSO);

			Ctx.SetComputeRootCBV(GlobalRootSigSlots::RS_VIEW_BUF, ClearBuf);

			Ctx.Dispatch(DivideRoundUp(G.ScreenWidth, 8u), DivideRoundUp(G.ScreenHeight, 8u), 1u);
		});
	}

	
	RenderGraphResourceHandle_t SceneColorLDR = RGBuilder.CreateTexture(G.ScreenWidth, G.ScreenHeight, RenderFormat::R8G8B8A8_UNORM, RenderGraphResourceAccessType_e::RTV | RenderGraphResourceAccessType_e::SRV, L"SceneColorLDRTexture");

	struct DeferredConstants_s
	{
		matrix CamToWorld;
		matrix PrevCamToWorld;

		uint32_t SceneColorTextureIndex;
		uint32_t SceneNormalTextureIndex;
		uint32_t SceneRoughnessMetallicTextureIndex;
		uint32_t DepthTextureIndex;

		uint32_t DrawMode;
		float3 CamPosition;

		uint32_t ShadowTexture;
		float3 SunDirection;

		uint32_t VelocityTextureIndex;
		uint32_t ConfidenceTextureIndex;
		float2 ViewportSizeRcp;

		uint32_t STAOTextureIndex;
		float __Pad[3];
	};

	if (G.DrawMode != 0 && G.DrawMode != 7)
	{
		static const float ProjectionA = 1000.0f / (1000.0f - 0.1f);
		static const float ProjectionB = (-1000.0f * 0.1f) / (1000.0f - 0.1f);

		RenderGraphResourceHandle_t STAOTexture = G.STAORenderer.GenerateSTAOTexture(RGBuilder, SceneDepthTexture, SceneNormalTexture, SceneVelocityTexture, ConfidenceTexture, G.Cam.GetProjection(), G.Cam.GetView(), uint2(G.ScreenWidth, G.ScreenHeight), NearPlaneZ);
		//RenderGraphResourceHandle_t STAOTexture = G.STReflectionRenderer.GenerateSTRTexture(RGBuilder, SceneDepthTexture, SceneColorTexture, SceneNormalTexture, G.Cam.GetProjection(), G.Cam.GetPixelProjection(), G.Cam.GetView(), uint2(G.ScreenWidth, G.ScreenHeight), NearPlaneZ);

		// Debug View
		RenderGraphPass_s& DebugViewPass = RGBuilder.AddPass(RenderGraphPassType_e::GRAPHICS, L"Debug View Pass")
		.AccessResource(SceneColorTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
		.AccessResource(SceneNormalTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
		.AccessResource(SceneRoughnessMetallicTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
		.AccessResource(SceneDepthTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
		.AccessResource(ShadowTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
		.AccessResource(SceneVelocityTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
		.AccessResource(ConfidenceTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
		.AccessResource(STAOTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
		.AccessResource(SceneColorLDR, RenderGraphResourceAccessType_e::RTV, RenderGraphLoadOp_e::DONT_CARE)
		.SetExecuteCallback([=](RenderGraph_s& RG, GPUContext_s& Ctx)
		{
			DeferredConstants_s DeferredConsts;

			DeferredConsts.CamToWorld = InverseMatrix(ViewProjection);
			DeferredConsts.PrevCamToWorld = InverseMatrix(G.PrevViewProjection);
			DeferredConsts.SceneColorTextureIndex = GetDescriptorIndex(RG.GetSRV(SceneColorTexture));
			DeferredConsts.SceneNormalTextureIndex = GetDescriptorIndex(RG.GetSRV(SceneNormalTexture));
			DeferredConsts.SceneRoughnessMetallicTextureIndex = GetDescriptorIndex(RG.GetSRV(SceneRoughnessMetallicTexture));
			DeferredConsts.DepthTextureIndex = GetDescriptorIndex(RG.GetSRV(SceneDepthTexture));
			DeferredConsts.DrawMode = G.DrawMode;
			DeferredConsts.CamPosition = G.Cam.GetPosition();
			DeferredConsts.ShadowTexture = GetDescriptorIndex(RG.GetSRV(ShadowTexture));
			DeferredConsts.SunDirection = SunDirection;
			DeferredConsts.VelocityTextureIndex = GetDescriptorIndex(RG.GetSRV(SceneVelocityTexture));
			DeferredConsts.ConfidenceTextureIndex = GetDescriptorIndex(RG.GetSRV(ConfidenceTexture));
			DeferredConsts.ViewportSizeRcp = float2(1.0f / (float)G.ScreenWidth, 1.0f / (float)G.ScreenHeight);
			DeferredConsts.STAOTextureIndex = GetDescriptorIndex(RG.GetSRV(STAOTexture));

			DynamicBuffer_t DeferredCBuf = CreateDynamicConstantBuffer(&DeferredConsts);

			FullScreenPassVSPS(RG, Ctx, SceneColorLDR, G.DebugViewPSO, DeferredCBuf);
		});
	}
	else
	{
		// Deferred
		RenderGraphPass_s& DeferredPass = RGBuilder.AddPass(RenderGraphPassType_e::GRAPHICS, L"Deferred Pass")
		.AccessResource(SceneColorTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
		.AccessResource(SceneNormalTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
		.AccessResource(SceneRoughnessMetallicTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
		.AccessResource(SceneDepthTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
		.AccessResource(ShadowTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
		.AccessResource(SceneVelocityTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
		.AccessResource(ConfidenceTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
		.AccessResource(SceneColorLDR, RenderGraphResourceAccessType_e::RTV, RenderGraphLoadOp_e::DONT_CARE)
		.SetExecuteCallback([=](RenderGraph_s& RG, GPUContext_s& Ctx)
		{
			DeferredConstants_s DeferredConsts;

			DeferredConsts.CamToWorld = InverseMatrix(ViewProjection);
			DeferredConsts.PrevCamToWorld = InverseMatrix(G.PrevViewProjection);
			DeferredConsts.SceneColorTextureIndex = GetDescriptorIndex(RG.GetSRV(SceneColorTexture));
			DeferredConsts.SceneNormalTextureIndex = GetDescriptorIndex(RG.GetSRV(SceneNormalTexture));
			DeferredConsts.SceneRoughnessMetallicTextureIndex = GetDescriptorIndex(RG.GetSRV(SceneRoughnessMetallicTexture));
			DeferredConsts.DepthTextureIndex = GetDescriptorIndex(RG.GetSRV(SceneDepthTexture));
			DeferredConsts.DrawMode = G.DrawMode;
			DeferredConsts.CamPosition = G.Cam.GetPosition();
			DeferredConsts.ShadowTexture = GetDescriptorIndex(RG.GetSRV(ShadowTexture));
			DeferredConsts.SunDirection = SunDirection;
			DeferredConsts.VelocityTextureIndex = GetDescriptorIndex(RG.GetSRV(SceneVelocityTexture));
			DeferredConsts.ConfidenceTextureIndex = GetDescriptorIndex(RG.GetSRV(ConfidenceTexture));
			DeferredConsts.ViewportSizeRcp = float2(1.0f / (float)G.ScreenWidth, 1.0f / (float)G.ScreenHeight);
			DeferredConsts.STAOTextureIndex = 0;

			DynamicBuffer_t DeferredCBuf = CreateDynamicConstantBuffer(&DeferredConsts);

			FullScreenPassVSPS(RG, Ctx, SceneColorLDR, G.DeferredPSO, DeferredCBuf);
		});
	}

	RenderGraphResourceHandle_t BackBufferTexture = RGBuilder.RefBackBufferTexture(View->GetCurrentBackBufferTexture(), View->GetCurrentBackBufferRTV(), rl::ResourceTransitionState::RENDER_TARGET);

	RenderGraphPass_s& DeferredPass = RGBuilder.AddPass(RenderGraphPassType_e::GRAPHICS, L"Tonemapper (U2)")
	.AccessResource(SceneColorLDR, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(BackBufferTexture, RenderGraphResourceAccessType_e::RTV, RenderGraphLoadOp_e::DONT_CARE)
	.SetExecuteCallback([=](RenderGraph_s& RG, GPUContext_s& Ctx)
	{
		struct
		{
			uint32_t InputTexture;
			float WhitePoint;
			float ExposureBias;
			float __Pad;
		} Uniforms;

		Uniforms.InputTexture = RG.GetSRVIndex(SceneColorLDR);
		Uniforms.WhitePoint = G.WhitePoint;
		Uniforms.ExposureBias = G.ExposureBias;

		DynamicBuffer_t TonemapCBuf = CreateDynamicConstantBuffer(&Uniforms);

		FullScreenPassVSPS(RG, Ctx, BackBufferTexture, WantsTonemap() ? G.U2TonemapPSO : G.NoTonemapPSO, TonemapCBuf);
	});

	RenderGraph_s Graph = RGBuilder.Build();

	Graph.Execute(clGroup);

	G.PrevViewProjection = ViewProjection;
}

void ShutdownApp()
{

}
