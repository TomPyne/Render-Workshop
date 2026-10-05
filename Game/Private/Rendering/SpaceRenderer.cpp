#include "Rendering/SpaceRenderer.h"

#include "Assets/AssetManager.h"
#include "Assets/TextureManager.h"
#include "Bloom.h"
#include "Object/CameraComponent.h"
#include "Object/ObjectComponent.h"
#include "Rendering/IRenderable.h"
#include "Rendering/Mesh.h"
#include "Rendering/Texture.h"
#include "Space/Space.h"
#include "Tools/GameStats.h"

#include <Render/Render.h>
#include <RenderUtils/GPUContext/GPUContext.h>
#include <RenderUtils/RenderPasses/RaytracingBuildPass.h>
#include <RenderUtils/RenderPasses/Tonemapping.h>
#include <Shared/FileUtils/PathUtils.h>
#include <Shared/Logging/Logging.h>

#include <SurfMath.h>

static struct SpaceRendererPrivate_s
{
	rl::RootSignaturePtr RootSignature;
	rl::GraphicsPipelineStatePtr DeferredPSO;
	rl::ComputePipelineStatePtr ShadowPSO;
	rl::ComputePipelineStatePtr ShadowTemporalPSO;
	TonemapRenderer_s TonemapRenderer;
	DebugViewRenderer_s DebugViewRenderer;
	DistanceFieldVisualiseRenderer_s DistanceFieldVisualiseRenderer;
	DistanceFieldAORenderer_s DistanceFieldAORenderer;
	BloomRenderer_s BloomRenderer;
	bool Initialized = false;
} G;

struct SpaceViewUniforms_s
{
	matrix ViewProjection;
	matrix PrevViewProjection;

	float3 CamPos;
	float Time;

	float2 InvViewportSize;
	float2 Pad0;
};

void SpaceRenderer_c::Init()
{
	ASSERTMSG(G.Initialized == false, "Space Renderer has already been initialized");

	rl::RootSignatureDesc RootSigDesc = {};
	RootSigDesc.Slots.resize(SpaceRendererRootSigSlots::RS_COUNT);
	RootSigDesc.Slots[SpaceRendererRootSigSlots::RS_DRAWCONSTANTS] = rl::RootSignatureSlot::CBVSlot(SpaceRendererCBVRegister::CBV_DRAWCONSTANTS, 0);
	RootSigDesc.Slots[SpaceRendererRootSigSlots::RS_VIEW_BUF] = rl::RootSignatureSlot::CBVSlot(SpaceRendererCBVRegister::CBV_VIEW_BUF, 0);
	RootSigDesc.Slots[SpaceRendererRootSigSlots::RS_MODEL_BUF] = rl::RootSignatureSlot::CBVSlot(SpaceRendererCBVRegister::CBV_MODEL_BUF, 0);
	RootSigDesc.Slots[SpaceRendererRootSigSlots::RS_MAT_BUF] = rl::RootSignatureSlot::CBVSlot(SpaceRendererCBVRegister::CBV_MAT_BUF, 0);
	RootSigDesc.Slots[SpaceRendererRootSigSlots::RS_TLAS] = rl::RootSignatureSlot::SRVSlot(0, 0); // Sticking the TLAS here makes all my other SRVs need to start at t1, SM6.6 allows a workaround with ResourceDescriptorHeap
	RootSigDesc.Slots[SpaceRendererRootSigSlots::RS_SRV_TABLE] = rl::RootSignatureSlot::DescriptorTableSlot(1, 0, rl::RootSignatureDescriptorTableType::SRV);
	RootSigDesc.Slots[SpaceRendererRootSigSlots::RS_UAV_TABLE] = rl::RootSignatureSlot::DescriptorTableSlot(0, 0, rl::RootSignatureDescriptorTableType::UAV);

	RootSigDesc.GlobalSamplers.resize(2);
	RootSigDesc.GlobalSamplers[0].AddressModeUVW(rl::SamplerAddressMode::WRAP).FilterModeMinMagMip(rl::SamplerFilterMode::ANISOTROPIC);
	RootSigDesc.GlobalSamplers[1].AddressModeUVW(rl::SamplerAddressMode::CLAMP).FilterModeMinMagMip(rl::SamplerFilterMode::LINEAR);

	G.RootSignature = rl::CreateRootSignature(RootSigDesc);

	G.TonemapRenderer.Init(G.RootSignature, SpaceRendererRootSigSlots::RS_VIEW_BUF, SpaceRendererCBVRegister::CBV_VIEW_BUF, SpaceRendererRootSigSlots::RS_SRV_TABLE);
	G.DebugViewRenderer.Init();
	G.DistanceFieldVisualiseRenderer.Init();
	G.DistanceFieldAORenderer.Init();
	G.BloomRenderer.Init();

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

		ASSERTMSG(G.DeferredPSO.IsValid(), "Failed to create Deferred PSO");
	}

	// Shadow PSO
	{
		static const Path_s ShadowCSPath = Path_s(PathDirectory_e::Shaders, L"Game", L"Shadows/RTShadowsInline.hlsl");
		rl::ComputePipelineStateDesc PsoDesc = {};
		PsoDesc.Cs = rl::CreateComputeShader(ShadowCSPath.ToString().c_str());
		PsoDesc.RootSignatureOverride = G.RootSignature;
		PsoDesc.DebugName = L"RTShadowsInline";

		G.ShadowPSO = rl::CreateComputePipelineState(PsoDesc);

		ASSERTMSG(G.ShadowPSO.IsValid(), "Failed to create Shadow PSO");
	}

	// Shadow Temporal PSO
	{
		static const Path_s ShadowTemporalCSPath = Path_s(PathDirectory_e::Shaders, L"Game", L"Shadows/ShadowTemporal.hlsl");
		rl::ComputePipelineStateDesc PsoDesc = {};
		PsoDesc.Cs = rl::CreateComputeShader(ShadowTemporalCSPath.ToString().c_str());
		PsoDesc.RootSignatureOverride = G.RootSignature;
		PsoDesc.DebugName = L"ShadowTemporal";

		G.ShadowTemporalPSO = rl::CreateComputePipelineState(PsoDesc);

		ASSERTMSG(G.ShadowTemporalPSO.IsValid(), "Failed to create Shadow Temporal PSO");
	}

	Clock = {};

	if (rl::Render_SupportsRaytracing())
	{
		RTScene = rl::CreateRaytracingScene();
	}
	else
	{
		LOGINFO("[SpaceRenderer_c::Init] Raytracing not supported, acceleration structures will not be built");
	}

	BlueNoiseTexture = TextureManager::RequestTexture(Path_s(PathDirectory_e::Assets, L"Game", L"Textures/BlueNoise.hp_tex"), false, true);

	GlobalDistanceField.Init(G.RootSignature, SpaceRendererRootSigSlots::RS_DRAWCONSTANTS, SpaceRendererRootSigSlots::RS_UAV_TABLE, SpaceRendererRootSigSlots::RS_SRV_TABLE);

	G.Initialized = true;
}

FrameBufferAlloc_s SpatialRenderingCollector_s::Alloc(const void* Data, uint32_t Size)
{
	return FrameBuffer.Alloc(Data, Size);
}

void SpaceRenderer_c::RenderSpace(const SpaceRendererScreenInfo_s& Screen, Space_c* Space, rl::CommandListSubmissionGroup& clGroup)
{
	if (!Space)
		return;

	CameraComponent_c* PrimaryCamera = Space->GetCamera();
	if (!PrimaryCamera)
		return;

	GameStats::UpdateCamera(PrimaryCamera);

	matrix ProjectionMatrix = PrimaryCamera->CalculateProjectionMatrix(Screen.Width, Screen.Height);
	matrix ViewMatrix = PrimaryCamera->CalculateViewMatrix();

	RenderGraphBuilder_s RGBuilder(RenderGraphResourcePool);

	FrameIndex++;

	SpatialRenderingCollector_s Collector(RGBuilder.GetMainFrameBuffer(), FrameIndex);

	for (IRenderable_c* Renderable : Space->RenderableComponents)
	{
		Renderable->Render(Collector);
	}

	GameStats::UpdatePrimCount(static_cast<uint32_t>(Collector.MainPass.Batches.size()));

	DistanceFieldInstances.clear();
	for (IRenderable_c* Renderable : Space->RenderableComponents)
	{
		Renderable->CollectDistanceFieldInstances(DistanceFieldInstances);
	}
	DistanceFieldScene.Update(DistanceFieldInstances);

	Clock.Tick();

	if (RTScene)
	{
		std::vector<Mesh_s*> MeshesToBuild;
		AssetManager_c::Get().CollectMeshesForRTBuild(MeshesToBuild);

		std::vector<rl::RaytracingGeometry_t> Geometries;
		Geometries.reserve(MeshesToBuild.size());
		for (Mesh_s* Mesh : MeshesToBuild)
		{
			if (Mesh->RTGeom)
			{
				Geometries.push_back(Mesh->RTGeom);
			}
		}

		// New geometry has no instances in the current scene, so it always needs a rebuild
		if (!Geometries.empty() || Space->RenderSceneDirty)
		{
			std::vector<rl::RaytracingInstance> Instances;
			for (IRenderable_c* Renderable : Space->RenderableComponents)
			{
				Renderable->CollectRaytracingInstances(Instances);
			}

			AddRaytracingBuildPass(RGBuilder, RGBuilder.ImportRaytracingScene(RTScene, L"RaytracingScene"), Geometries, Instances);
		}
	}

	Space->RenderSceneDirty = false;

	const matrix ViewProjection = ViewMatrix * ProjectionMatrix;
	const matrix InverseViewProjection = InverseMatrix(ViewProjection);

	if (Space->ActiveCameraChanged)
	{
		ResetTemporalHistory();
		Space->ActiveCameraChanged = false;
	}

	if (!HasPrevView)
	{
		PrevViewProjection = ViewProjection;
		HasPrevView = true;
	}

	// TODO: Create directional light actor
	const float3 LightRadiance = float3(1.0f, 0.95f, 0.85f) * 3.0f;
	const float3 AmbientColor = float3(0.25f, 0.3f, 0.4f) * 0.3f;
	const float SunSoftAngle = 0.02f;
	const float LightPitch = ConvertToRadians(-18.37f);
	const float LightYaw = ConvertToRadians(50.0f);

	const float3 LightDirection = -float3(
		cosf(LightPitch) * sinf(LightYaw),
		sinf(LightPitch),
		cosf(LightPitch) * cosf(LightYaw)
	);

	SpaceViewUniforms_s ViewUniforms = {};
	ViewUniforms.ViewProjection = ViewProjection;
	ViewUniforms.PrevViewProjection = PrevViewProjection;
	ViewUniforms.CamPos = PrimaryCamera->GetWorldPosition();
	ViewUniforms.Time = Clock.GetTotalSeconds();
	ViewUniforms.InvViewportSize = float2(1.0f / Screen.Width, 1.0f / Screen.Height);

	FrameBufferAlloc_s ViewUniformsBuffer = RGBuilder.Alloc(ViewUniforms);

	RenderGraphTextureDesc_s GBufferTextureDesc = {};
	GBufferTextureDesc.Width = Screen.Width;
	GBufferTextureDesc.Height = Screen.Height;
	GBufferTextureDesc.Format = rl::RenderFormat::R16G16B16A16_FLOAT;
	GBufferTextureDesc.AccessTypes = RenderGraphResourceAccessType_e::RTV | RenderGraphResourceAccessType_e::SRV;

	RenderGraphResourceHandle_t SceneColorMetallicTexture = RGBuilder.CreateTexture(GBufferTextureDesc, L"SceneColorMetallicTexture");
	RenderGraphResourceHandle_t SceneNormalRoughnessTexture = RGBuilder.CreateTexture(GBufferTextureDesc, L"SceneNormalRoughnessTexture");
	RenderGraphResourceHandle_t SceneEmissiveSpecularTexture = RGBuilder.CreateTexture(GBufferTextureDesc, L"SceneEmissiveSpecularTexture");	
	RenderGraphResourceHandle_t SceneVelocityTexture = RGBuilder.CreateTexture(GBufferTextureDesc, L"SceneVelocityTexture");

	RenderGraphTextureDesc_s AOSceneTextureDesc = GBufferTextureDesc;
	AOSceneTextureDesc.Format = rl::RenderFormat::R8_UNORM;
	AOSceneTextureDesc.ClearValue = float4(1.f, 0.f, 0.f, 0.f);
	RenderGraphResourceHandle_t SceneAOTexture = RGBuilder.CreateTexture(AOSceneTextureDesc, L"SceneAOTexture");

	RenderGraphTextureDesc_s SceneDepthTextureDesc = AOSceneTextureDesc;
	SceneDepthTextureDesc.Format = rl::RenderFormat::R32_FLOAT;
	SceneDepthTextureDesc.AccessTypes = RenderGraphResourceAccessType_e::DSV | RenderGraphResourceAccessType_e::SRV;
	RenderGraphResourceHandle_t SceneDepthTexture = RGBuilder.CreateTexture(SceneDepthTextureDesc, L"SceneDepthTexture");

	RenderGraphPass_s& MeshDrawPass = RGBuilder.AddPass(RenderGraphPassType_e::GRAPHICS, L"Mesh Pass")
	.AccessResource(SceneColorMetallicTexture, RenderGraphResourceAccessType_e::RTV, RenderGraphLoadOp_e::CLEAR)
	.AccessResource(SceneNormalRoughnessTexture, RenderGraphResourceAccessType_e::RTV, RenderGraphLoadOp_e::CLEAR)
	.AccessResource(SceneEmissiveSpecularTexture, RenderGraphResourceAccessType_e::RTV, RenderGraphLoadOp_e::CLEAR)
	.AccessResource(SceneAOTexture, RenderGraphResourceAccessType_e::RTV, RenderGraphLoadOp_e::CLEAR)
	.AccessResource(SceneVelocityTexture, RenderGraphResourceAccessType_e::RTV, RenderGraphLoadOp_e::CLEAR)
	.AccessResource(SceneDepthTexture, RenderGraphResourceAccessType_e::DSV, RenderGraphLoadOp_e::CLEAR)
	.SetExecuteCallback([=, &Collector](RenderGraph_s& RG, GPUContext_s& Ctx)
	{
		Ctx.SetRootSignature(G.RootSignature);
		rl::RenderTargetView_t SceneRTVs[] =
		{
			RG.GetRTV(SceneColorMetallicTexture),
			RG.GetRTV(SceneNormalRoughnessTexture),
			RG.GetRTV(SceneEmissiveSpecularTexture),
			RG.GetRTV(SceneAOTexture),
			RG.GetRTV(SceneVelocityTexture),
		};

		rl::DepthStencilView_t SceneDSV = RG.GetDSV(SceneDepthTexture);
		Ctx.SetRenderTargets(SceneRTVs, ARRAYSIZE(SceneRTVs), SceneDSV); // TODO: this should be set by the graph

		rl::Viewport vp{ Screen.Width, Screen.Height };
		Ctx.SetViewports(&vp, 1);
		Ctx.SetDefaultScissor(); // Could also be captured by the command context

		Ctx.SetGraphicsRootCBV(SpaceRendererRootSigSlots::RS_VIEW_BUF, ViewUniformsBuffer);
		Ctx.SetGraphicsRootDescriptorTable(SpaceRendererRootSigSlots::RS_SRV_TABLE); // Root sig stuff is trickier

		int index = 0;
		for (const SpatialRenderingBatch_s& Batch : Collector.MainPass.Batches)
		{
			Ctx.SetPipelineState(Batch.PSO); // TODO: check when PSO has changed in the command list
			Ctx.SetGraphicsRootCBV(SpaceRendererRootSigSlots::RS_DRAWCONSTANTS, Batch.DynamicUniforms);
			Ctx.SetGraphicsRootCBV(SpaceRendererRootSigSlots::RS_MODEL_BUF, Batch.MeshUniforms);
			Ctx.SetGraphicsRootCBV(SpaceRendererRootSigSlots::RS_MAT_BUF, Batch.MaterialUniforms);

			Ctx.SetIndexBuffer(Batch.IndexBuffer, Batch.IndexBufferFormat, 0);
			Ctx.DrawIndexedInstanced(Batch.IndexCount, 1, Batch.IndexOffset, 0, 0);
			index++;
		}
	});

	RenderGraphResourceHandle_t RaytracingSceneResource = RGBuilder.ImportRaytracingScene(RTScene, L"RaytracingScene");
	RenderGraphResourceHandle_t ShadowTexture = RGBuilder.CreateTexture(Screen.Width, Screen.Height, rl::RenderFormat::R8_UNORM, RenderGraphResourceAccessType_e::SRV_UAV, L"ShadowTexture");

	uint32_t BlueNoiseSrvIndex = BlueNoiseTexture && BlueNoiseTexture->IsReady() ? rl::GetDescriptorIndex(BlueNoiseTexture->SRV) : 0;
	RenderGraphPass_s& ShadowPass = RGBuilder.AddPass(RenderGraphPassType_e::COMPUTE, L"Shadow Pass")
	.AccessResource(RaytracingSceneResource, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(SceneDepthTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(ShadowTexture, RenderGraphResourceAccessType_e::UAV, RenderGraphLoadOp_e::DONT_CARE)
	.SetExecuteCallback([=, &InverseViewProjection](RenderGraph_s& RG, GPUContext_s& Ctx)
	{
		struct ShadowUniforms_s
		{
			matrix CamToWorld;
			
			float3 SunDirection;
			float SunSoftAngle;

			float2 ScreenResolution;
			uint32_t SceneDepthTexture;
			uint32_t SceneShadowTexture;

			uint32_t BlueNoiseTexture;
			uint32_t FrameID;
			float2 __Pad;
		};

		ShadowUniforms_s Uniforms;
		Uniforms.CamToWorld = InverseViewProjection;
		Uniforms.SunDirection = LightDirection;
		Uniforms.SunSoftAngle = SunSoftAngle;
		Uniforms.ScreenResolution = float2(static_cast<float>(Screen.Width), static_cast<float>(Screen.Height));
		Uniforms.SceneDepthTexture = RG.GetSRVIndex(SceneDepthTexture);
		Uniforms.SceneShadowTexture = RG.GetUAVIndex(ShadowTexture);
		Uniforms.BlueNoiseTexture = BlueNoiseSrvIndex;
		Uniforms.FrameID = static_cast<uint32_t>(FrameIndex);

		Ctx.SetRootSignature(G.RootSignature);
		Ctx.SetComputeRootDescriptorTable(SpaceRendererRootSigSlots::RS_UAV_TABLE);
		Ctx.SetComputeRootDescriptorTable(SpaceRendererRootSigSlots::RS_SRV_TABLE);

		Ctx.SetPipelineState(G.ShadowPSO);

		Ctx.SetComputeRootCBV(SpaceRendererRootSigSlots::RS_DRAWCONSTANTS, RG.Alloc(Uniforms));
		Ctx.SetComputeRootSRV(SpaceRendererRootSigSlots::RS_TLAS, RG.GetRaytracingScene(RaytracingSceneResource));

		Ctx.Dispatch(DivideRoundUp(Screen.Width, 8u), DivideRoundUp(Screen.Height, 8u), 1u);
	});

	if (ShadowHistorySize.x != Screen.Width || ShadowHistorySize.y != Screen.Height)
	{
		for (uint32_t HistoryIt = 0; HistoryIt < 2; HistoryIt++)
		{
			ShadowHistoryTextures[HistoryIt] = CreateRenderGraphTexture(Screen.Width, Screen.Height, rl::RenderFormat::R16G16_FLOAT, RenderGraphResourceAccessType_e::SRV_UAV, L"ShadowHistory");
			LinearDepthHistoryTextures[HistoryIt] = CreateRenderGraphTexture(Screen.Width, Screen.Height, rl::RenderFormat::R32_FLOAT, RenderGraphResourceAccessType_e::SRV_UAV, L"LinearDepthHistory");
		}

		ShadowHistorySize = uint2(Screen.Width, Screen.Height);
		ShadowHistoryValid = false;
	}

	const uint32_t ShadowHistoryWriteIndex = ShadowHistoryReadIndex ^ 1u;

	RenderGraphResourceHandle_t ShadowHistoryTexture = RGBuilder.InjectTexture(ShadowHistoryTextures[ShadowHistoryReadIndex], L"ShadowHistory");
	RenderGraphResourceHandle_t LinearDepthHistoryTexture = RGBuilder.InjectTexture(LinearDepthHistoryTextures[ShadowHistoryReadIndex], L"LinearDepthHistory");
	RenderGraphResourceHandle_t AccumulatedShadowTexture = RGBuilder.InjectTexture(ShadowHistoryTextures[ShadowHistoryWriteIndex], L"AccumulatedShadow");
	RenderGraphResourceHandle_t LinearDepthTexture = RGBuilder.InjectTexture(LinearDepthHistoryTextures[ShadowHistoryWriteIndex], L"LinearDepth");

	const bool UseShadowHistory = ShadowHistoryValid;

	RenderGraphPass_s& ShadowTemporalPass = RGBuilder.AddPass(RenderGraphPassType_e::COMPUTE, L"Shadow Temporal Pass")
	.AccessResource(SceneDepthTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(SceneVelocityTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(ShadowTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(ShadowHistoryTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(LinearDepthHistoryTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(AccumulatedShadowTexture, RenderGraphResourceAccessType_e::UAV, RenderGraphLoadOp_e::DONT_CARE)
	.AccessResource(LinearDepthTexture, RenderGraphResourceAccessType_e::UAV, RenderGraphLoadOp_e::DONT_CARE)
	.SetExecuteCallback([=, &InverseViewProjection](RenderGraph_s& RG, GPUContext_s& Ctx)
	{
		struct ShadowTemporalUniforms_s
		{
			matrix CamToWorld;

			uint2 ViewportSize;
			float2 InvViewportSize;

			uint32_t SceneDepthTexture;
			uint32_t VelocityTexture;
			uint32_t CurrentShadowTexture;
			uint32_t HistoryShadowTexture;

			uint32_t HistoryLinearDepthTexture;
			uint32_t OutShadowTexture;
			uint32_t OutLinearDepthTexture;
			uint32_t HistoryValid;

			float MaxConfidence;
			float ConfidenceRate;
			float DepthTolerance;
			float __Pad;
		};
		static_assert(sizeof(ShadowTemporalUniforms_s) == 128, "Must match Uniforms_s in ShadowTemporal.hlsl");

		ShadowTemporalUniforms_s Uniforms;
		Uniforms.CamToWorld = InverseViewProjection;
		Uniforms.ViewportSize = uint2(Screen.Width, Screen.Height);
		Uniforms.InvViewportSize = float2(1.0f / Screen.Width, 1.0f / Screen.Height);
		Uniforms.SceneDepthTexture = RG.GetSRVIndex(SceneDepthTexture);
		Uniforms.VelocityTexture = RG.GetSRVIndex(SceneVelocityTexture);
		Uniforms.CurrentShadowTexture = RG.GetSRVIndex(ShadowTexture);
		Uniforms.HistoryShadowTexture = RG.GetSRVIndex(ShadowHistoryTexture);
		Uniforms.HistoryLinearDepthTexture = RG.GetSRVIndex(LinearDepthHistoryTexture);
		Uniforms.OutShadowTexture = RG.GetUAVIndex(AccumulatedShadowTexture);
		Uniforms.OutLinearDepthTexture = RG.GetUAVIndex(LinearDepthTexture);
		Uniforms.HistoryValid = UseShadowHistory ? 1u : 0u;
		Uniforms.MaxConfidence = ShadowTemporalMaxConfidence;
		Uniforms.ConfidenceRate = ShadowTemporalConfidenceRate;
		Uniforms.DepthTolerance = ShadowTemporalDepthTolerance;
		Uniforms.__Pad = 0.0f;

		Ctx.SetRootSignature(G.RootSignature);
		Ctx.SetComputeRootDescriptorTable(SpaceRendererRootSigSlots::RS_UAV_TABLE);
		Ctx.SetComputeRootDescriptorTable(SpaceRendererRootSigSlots::RS_SRV_TABLE);

		Ctx.SetPipelineState(G.ShadowTemporalPSO);

		Ctx.SetComputeRootCBV(SpaceRendererRootSigSlots::RS_DRAWCONSTANTS, RG.Alloc(Uniforms));

		Ctx.Dispatch(DivideRoundUp(Screen.Width, 8u), DivideRoundUp(Screen.Height, 8u), 1u);
	});

	const bool IsDistanceFieldView = DebugViewMode == DebugViewMode_e::GlobalDistanceFieldSlice || DebugViewMode == DebugViewMode_e::GlobalDistanceField;

	RenderGraphResourceHandle_t GlobalVolume = {};
	if (DistanceFieldScene.GetInstanceCount() > 0 || IsDistanceFieldView)
	{
		GlobalVolume = GlobalDistanceField.AddPasses(RGBuilder, PrimaryCamera->GetWorldPosition(), DistanceFieldScene);
	}

	RenderGraphResourceHandle_t DistanceFieldAOTexture = G.DistanceFieldAORenderer.AddPass(RGBuilder, DistanceFieldAO, GlobalDistanceField, GlobalVolume,
		SceneDepthTexture, SceneNormalRoughnessTexture, InverseViewProjection, uint2(Screen.Width, Screen.Height));

	RenderGraphResourceHandle_t LitTexture = RGBuilder.CreateTexture(Screen.Width, Screen.Height, rl::RenderFormat::R11G11B10_FLOAT, RenderGraphResourceAccessType_e::SRV_UAV_RTV,  L"LitTexture");

	RenderGraphPass_s& DeferredPass = RGBuilder.AddPass(RenderGraphPassType_e::GRAPHICS, L"Deferred Pass")
	.AccessResource(SceneColorMetallicTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(SceneNormalRoughnessTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(SceneEmissiveSpecularTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(SceneAOTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(SceneDepthTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(AccumulatedShadowTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(LitTexture, RenderGraphResourceAccessType_e::RTV, RenderGraphLoadOp_e::DONT_CARE)
	.SetExecuteCallback([=, &InverseViewProjection](RenderGraph_s& RG, GPUContext_s& Ctx)
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
			uint32_t ShadowTextureIndex;
			uint32_t SceneAOTextureIndex;
			float __Pad;
		};
		static_assert(sizeof(DeferredConstants_s) == 128, "Must match DeferredData_s in Deferred.hlsl");

		FrameBufferAlloc_s DeferredCBuf;
		DeferredConstants_s* Uniforms = RG.Alloc<DeferredConstants_s>(DeferredCBuf);

		// TODO: replace with a light component
		Uniforms->InvViewProjection = InverseViewProjection;
		Uniforms->LightDirection = LightDirection;
		Uniforms->LightRadiance = LightRadiance;
		Uniforms->AmbientColor = AmbientColor;

		Uniforms->SceneColorMetallicTextureIndex = RG.GetSRVIndex(SceneColorMetallicTexture);
		Uniforms->SceneNormalRoughnessTextureIndex = RG.GetSRVIndex(SceneNormalRoughnessTexture);
		Uniforms->SceneEmissiveSpecularTextureIndex = RG.GetSRVIndex(SceneEmissiveSpecularTexture);
		Uniforms->SceneDepthTextureIndex = RG.GetSRVIndex(SceneDepthTexture);
		Uniforms->ShadowTextureIndex = RG.GetSRVIndex(AccumulatedShadowTexture);
		Uniforms->SceneAOTextureIndex = RG.GetSRVIndex(SceneAOTexture);

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

	if (DebugViewMode == DebugViewMode_e::Lit)
	{
		G.BloomRenderer.AddPass(RGBuilder, LitTexture, uint2(Screen.Width, Screen.Height));

		G.TonemapRenderer.AddPass(RGBuilder, TonemapMode_e::ACES, LitTexture, BackBufferTexture);
	}
	else if (IsDistanceFieldView)
	{
		const DistanceFieldVisualiseMode_e VisualiseMode = DebugViewMode == DebugViewMode_e::GlobalDistanceFieldSlice ? DistanceFieldVisualiseMode_e::GlobalSlice : DistanceFieldVisualiseMode_e::GlobalTrace;
		const RenderGraphResourceHandle_t VisualiseTexture = G.DistanceFieldVisualiseRenderer.AddPass(RGBuilder, VisualiseMode, DistanceFieldVisualise, GlobalDistanceField, GlobalVolume, InverseViewProjection, uint2(Screen.Width, Screen.Height));

		// The visualisation is already display ready, so this is a straight copy
		G.TonemapRenderer.AddPass(RGBuilder, TonemapMode_e::None, VisualiseTexture, BackBufferTexture);
	}
	else
	{
		DebugViewInputs_s DebugViewInputs = {};
		DebugViewInputs.SceneColorMetallic = SceneColorMetallicTexture;
		DebugViewInputs.SceneNormalRoughness = SceneNormalRoughnessTexture;
		DebugViewInputs.SceneEmissiveSpecular = SceneEmissiveSpecularTexture;
		DebugViewInputs.SceneDepth = SceneDepthTexture;
		DebugViewInputs.SceneAO = SceneAOTexture;
		DebugViewInputs.DistanceFieldAO = DistanceFieldAOTexture;
		DebugViewInputs.Frame = static_cast<uint32_t>(FrameIndex);
		DebugViewInputs.Time = Clock.GetTotalSeconds();

		G.DebugViewRenderer.AddPass(RGBuilder, DebugViewMode, DebugViewInputs, BackBufferTexture);
	}

	RenderGraph_s Graph = RGBuilder.Build();

	Graph.Execute(&clGroup);

	PrevViewProjection = ViewProjection;

	ShadowHistoryReadIndex = ShadowHistoryWriteIndex;
	ShadowHistoryValid = true;
}

void SpaceRenderer_c::ResetTemporalHistory()
{
	HasPrevView = false;
	ShadowHistoryValid = false;
}

rl::RootSignature_t SpaceRenderer_c::GetRootSignature()
{
	ASSERTMSG(G.RootSignature.IsValid(), "SpaceRenderer has not been initialized");

	return G.RootSignature;
}

const rl::GraphicsPipelineTargetDesc& SpaceRenderer_c::GetMaterialPipelineTargetDesc()
{
	static rl::GraphicsPipelineTargetDesc MaterialPipelineTargetDesc = rl::GraphicsPipelineTargetDesc(
		{ 
			rl::RenderFormat::R16G16B16A16_FLOAT, // Albedo + Metallic
			rl::RenderFormat::R16G16B16A16_FLOAT, // Normal + Roughness
			rl::RenderFormat::R16G16B16A16_FLOAT, // Emissive + Specular
			rl::RenderFormat::R8_UNORM, // AO
			rl::RenderFormat::R16G16B16A16_FLOAT, // Velocity + previous view depth
		},
		{
			rl::BlendMode::None(),
			rl::BlendMode::None(),
			rl::BlendMode::None(),
			rl::BlendMode::None(),
			rl::BlendMode::None(),
		}, 
		rl::RenderFormat::D32_FLOAT);
	return MaterialPipelineTargetDesc;
}
