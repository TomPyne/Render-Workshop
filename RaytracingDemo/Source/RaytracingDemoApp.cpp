#include <Render/Render.h>
#include <SurfMath.h>
#include "imgui.h"

#include <Assets/Assets.h>
#include <Camera/FlyCamera.h>
#include <Logging/Logging.h>
#include <ModelUtils/Model.h>
#include <Materials/Materials.h>
#include <TextureUtils/TextureManager.h>

#include <HPModel.h>

#include <ppl.h>

using namespace tpr;

struct RTDTexture_s
{
	tpr::TexturePtr Texture = {};
	tpr::ShaderResourceViewPtr SRV = {};
};

struct RTDMaterialParams_s
{
	float3 Albedo = float3(1, 1, 1);
	uint32_t AlbedoTextureIndex = 0;
	uint32_t NormalTextureIndex = 0;
	float Roughness = 1.0f;
	float Metallic = 0.0f;
	uint32_t MetallicRoughnessTextureIndex = 0;
};

struct RTDMaterial_s
{
	RTDMaterialParams_s Params;

	RTDTexture_s* AlbedoTexture = nullptr;
	RTDTexture_s* NormalTexture = nullptr;
	RTDTexture_s* MetallicRoughnessTexture = nullptr;

	tpr::ConstantBuffer_t MaterialConstantBuffer = {};
};

struct RTDBuffer_s
{
	tpr::StructuredBufferPtr StructuredBuffer = {};
	tpr::ShaderResourceViewPtr BufferSRV = {};

	template<typename T>
	void Init(const T* const Data, size_t Count)
	{
		if (Count)
		{
			CHECK(Data != nullptr);
			StructuredBuffer = tpr::CreateStructuredBuffer(Data, Count);
			CHECK(StructuredBuffer);
			BufferSRV = tpr::CreateStructuredBufferSRV(StructuredBuffer, 0u, static_cast<uint32_t>(Count), static_cast<uint32_t>(sizeof(T)));
			CHECK(BufferSRV);
		}
	}
};

enum RTDDrawConstantSlots_e
{
	DCS_INDEX_OFFSET = 0,
	DCS_MESHLET_OFFSET = 0,
	DCS_COUNT,
};

struct RTDMeshConstants_s
{
	// Vertex data
	uint32_t PositionBufSRVIndex = 0;
	uint32_t NormalBufSRVIndex = 0;
	uint32_t TangentBufSRVIndex = 0;
	uint32_t BitangentBufSRVIndex = 0;
	uint32_t TexcoordBufSRVIndex = 0;
	uint32_t IndexBufSRVIndex = 0;
	// Mesh shader data
	uint32_t MeshletBufSRVIndex = 0;
	uint32_t UniqueVertexIndexBufSRVIndex = 0;
	uint32_t PrimitiveIndexBufSRVIndex = 0;

	float __pad[3];
};

struct RTDMesh_s
{
	uint32_t IndexOffset;
	uint32_t IndexCount;

	uint32_t MeshletOffset;
	uint32_t MeshletCount;

	RTDMaterial_s* Material = nullptr;
};

struct RTDModel_s
{
	RTDBuffer_s PositionBuffer;
	RTDBuffer_s NormalBuffer;
	RTDBuffer_s TangentBuffer;
	RTDBuffer_s BitangentBuffer;
	RTDBuffer_s TexcoordBuffer;
	RTDBuffer_s IndexBuffer;

	RTDBuffer_s MeshletBuffer;
	RTDBuffer_s UniqueVertexIndexBuffer;
	RTDBuffer_s PrimitiveIndexBuffer;

	tpr::ConstantBufferPtr ModelConstantBuffer;

	std::vector<RTDMesh_s> Meshes;

	void Init(const HPModel_s* Asset);

	void Draw(tpr::CommandList* CL) const;
};

enum RootSigSlots
{
	RS_DRAWCONSTANTS,
	RS_VIEW_BUF,
	RS_MODEL_BUF,
	RS_MAT_BUF,
	RS_SRV_TABLE,
	RS_UAV_TABLE,
	RS_COUNT,
};

struct SceneTarget_s
{
	TexturePtr Texture = {};
	RenderTargetViewPtr RTV = {};
	ShaderResourceViewPtr SRV = {};

	void Init(uint32_t Width, uint32_t Height, RenderFormat Format, const wchar_t* DebugName)
	{
		TextureCreateDescEx Desc = {};
		Desc.DebugName = DebugName ? DebugName : L"UnknownSceneTarget";
		Desc.Flags = RenderResourceFlags::RTV | RenderResourceFlags::SRV;
		Desc.ResourceFormat = Format;
		Desc.Height = Height;
		Desc.Width = Width;
		Desc.InitialState = ResourceTransitionState::RENDER_TARGET;
		Desc.Dimension = TextureDimension::TEX2D;
		Texture = CreateTextureEx(Desc);
		RTV = CreateTextureRTV(Texture, Format, TextureDimension::TEX2D, 1u);
		SRV = CreateTextureSRV(Texture, Format, TextureDimension::TEX2D, 1u, 1u);

		if (!Texture || !RTV || !SRV)
		{
			LOGERROR("Scene Target [%S] failed to intialize", DebugName ? DebugName : L"Unknown");
		}
	}
};

struct SceneDepth_s
{
	TexturePtr Texture = {};
	DepthStencilViewPtr DSV = {};
	ShaderResourceViewPtr SRV = {};

	void Init(uint32_t Width, uint32_t Height, RenderFormat DepthFormat, RenderFormat SRVFormat, const wchar_t* DebugName)
	{
		TextureCreateDescEx Desc = {};
		Desc.DebugName = DebugName ? DebugName : L"UnkownSceneDepth";
		Desc.Flags = RenderResourceFlags::DSV | RenderResourceFlags::SRV;
		Desc.ResourceFormat = DepthFormat;
		Desc.Height = Height;
		Desc.Width = Width;
		Desc.InitialState = ResourceTransitionState::DEPTH_WRITE;
		Desc.Dimension = TextureDimension::TEX2D;
		Texture = CreateTextureEx(Desc);
		DSV = CreateTextureDSV(Texture, DepthFormat, TextureDimension::TEX2D, 1u);
		SRV = CreateTextureSRV(Texture, SRVFormat, TextureDimension::TEX2D, 1u, 1u);
	}
};

struct Globals_s
{
	std::vector<RTDModel_s> Models;

	// Targets
	uint32_t ScreenWidth = 0;
	uint32_t ScreenHeight = 0;

	SceneTarget_s SceneColor = {};
	SceneTarget_s SceneNormal = {};
	SceneTarget_s SceneRoughnessMetallic = {};
	SceneDepth_s SceneDepth = {};

	// Camera
	FlyCamera Cam;

	// Shaders
	GraphicsPipelineStatePtr MeshVSPSO;
	GraphicsPipelineStatePtr MeshMSPSO;
	GraphicsPipelineStatePtr DeferredPSO;

	std::map<ModelAsset_s*, std::unique_ptr<RTDModel_s>> ModelMap;
	std::map<MaterialAsset_s*, std::unique_ptr<RTDMaterial_s>> MaterialMap;
	std::map<MaterialAsset_s*, std::unique_ptr<RTDTexture_s>> TextureMap;

	RTDMaterial_s DefaultMaterial = {};

	bool UseMeshShaders = false;
	bool ShowMeshID = false;
	int32_t DrawMode = 0;
} G;

void RTDModel_s::Init(const HPModel_s* Asset)
{
	CHECK(Asset != nullptr);

	CHECK(!Asset->Positions.empty());
	PositionBuffer.Init(Asset->Positions.data(), Asset->Positions.size());

	CHECK(!Asset->Indices.empty());
	if (Asset->IndexFormat == tpr::RenderFormat::R32_UINT)
	{
		IndexBuffer.Init(reinterpret_cast<const uint32_t*>(Asset->Indices.data()), Asset->Indices.size() / 4);
	}
	else // R16_UINT
	{
		IndexBuffer.Init(reinterpret_cast<const uint16_t*>(Asset->Indices.data()), Asset->Indices.size() / 2);
	}		

	if (Asset->HasNormals)
	{
		CHECK(!Asset->Normals.empty());
		NormalBuffer.Init(Asset->Normals.data(), Asset->Normals.size());
	}

	if (Asset->HasTangents)
	{
		CHECK(!Asset->Tangents.empty());
		TangentBuffer.Init(Asset->Tangents.data(), Asset->Tangents.size());
	}

	if (Asset->HasBitangents)
	{
		CHECK(!Asset->Bitangents.empty());
		BitangentBuffer.Init(Asset->Bitangents.data(), Asset->Bitangents.size());
	}

	if (Asset->HasTexcoords)
	{
		CHECK(!Asset->Texcoords.empty());
		TexcoordBuffer.Init(Asset->Texcoords.data(), Asset->Texcoords.size());
	}

	if (!Asset->Meshlets.empty())
	{
		MeshletBuffer.Init(Asset->Meshlets.data(), Asset->Meshlets.size());
	}

	if (!Asset->UniqueVertexIndices.empty())
	{
		UniqueVertexIndexBuffer.Init(reinterpret_cast<const uint32_t*>(Asset->UniqueVertexIndices.data()), Asset->UniqueVertexIndices.size() / 4);
	}

	if (!Asset->PrimitiveIndices.empty())
	{
		PrimitiveIndexBuffer.Init(Asset->PrimitiveIndices.data(), Asset->PrimitiveIndices.size());
	}

	RTDMeshConstants_s MeshConstants = {};
	MeshConstants.PositionBufSRVIndex = tpr::GetDescriptorIndex(PositionBuffer.BufferSRV);
	MeshConstants.NormalBufSRVIndex = tpr::GetDescriptorIndex(NormalBuffer.BufferSRV);
	MeshConstants.TangentBufSRVIndex = tpr::GetDescriptorIndex(TangentBuffer.BufferSRV);
	MeshConstants.BitangentBufSRVIndex = tpr::GetDescriptorIndex(BitangentBuffer.BufferSRV);
	MeshConstants.TexcoordBufSRVIndex = tpr::GetDescriptorIndex(TexcoordBuffer.BufferSRV);
	MeshConstants.IndexBufSRVIndex = tpr::GetDescriptorIndex(IndexBuffer.BufferSRV);

	MeshConstants.MeshletBufSRVIndex = tpr::GetDescriptorIndex(MeshletBuffer.BufferSRV);
	MeshConstants.UniqueVertexIndexBufSRVIndex = tpr::GetDescriptorIndex(UniqueVertexIndexBuffer.BufferSRV);
	MeshConstants.PrimitiveIndexBufSRVIndex = tpr::GetDescriptorIndex(PrimitiveIndexBuffer.BufferSRV);

	ModelConstantBuffer = tpr::CreateConstantBuffer(&MeshConstants);

	Meshes.reserve(Asset->Meshes.size());
	for (const HPModel_s::Mesh_s& MeshFromAsset : Asset->Meshes)
	{
		RTDMesh_s Mesh = {};
		Mesh.IndexCount = MeshFromAsset.IndexCount;
		Mesh.IndexOffset = MeshFromAsset.IndexOffset;
		Mesh.MeshletOffset = MeshFromAsset.MeshletOffset;
		Mesh.MeshletCount = MeshFromAsset.MeshletCount;		

		Meshes.push_back(Mesh);
	}
}

void RTDModel_s::Draw(tpr::CommandList* CL) const
{
	CL->SetGraphicsRootCBV(RS_MODEL_BUF, ModelConstantBuffer);

	if (G.UseMeshShaders)
	{
		for (const RTDMesh_s& Mesh : Meshes)
		{
			CL->SetGraphicsRootValue(RS_DRAWCONSTANTS, DCS_MESHLET_OFFSET, Mesh.MeshletOffset);

			CL->DispatchMesh(Mesh.MeshletCount, 1u, 1u);
		}
	}
	else
	{
		for (const RTDMesh_s& Mesh : Meshes)
		{
			CL->SetGraphicsRootValue(RS_DRAWCONSTANTS, DCS_INDEX_OFFSET, Mesh.IndexOffset);

			CL->DrawInstanced(Mesh.IndexCount, 1u, 0u, 0u);
		}
	}
}

tpr::RenderInitParams GetAppRenderParams()
{
	tpr::RenderInitParams Params;
#ifdef _DEBUG
	Params.DebugEnabled = false;
#else
	Params.DebugEnabled = false;
#endif

	Params.RootSigDesc.Flags = RootSignatureFlags::NONE;
	Params.RootSigDesc.Slots.resize(RS_COUNT);
	Params.RootSigDesc.Slots[RS_DRAWCONSTANTS] = RootSignatureSlot::ConstantsSlot(DCS_COUNT, 0);
	Params.RootSigDesc.Slots[RS_VIEW_BUF] = RootSignatureSlot::CBVSlot(1, 0);
	Params.RootSigDesc.Slots[RS_MODEL_BUF] = RootSignatureSlot::CBVSlot(2, 0);
	Params.RootSigDesc.Slots[RS_MAT_BUF] = RootSignatureSlot::CBVSlot(3, 0);
	Params.RootSigDesc.Slots[RS_SRV_TABLE] = RootSignatureSlot::DescriptorTableSlot(0, 0, tpr::RootSignatureDescriptorTableType::SRV);
	Params.RootSigDesc.Slots[RS_UAV_TABLE] = RootSignatureSlot::DescriptorTableSlot(0, 0, tpr::RootSignatureDescriptorTableType::UAV);

	Params.RootSigDesc.GlobalSamplers.resize(2);
	Params.RootSigDesc.GlobalSamplers[0].AddressModeUVW(SamplerAddressMode::WRAP).FilterModeMinMagMip(SamplerFilterMode::LINEAR);
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

	// First cook

	std::vector<std::wstring> ModelPaths =
	{
		L"Cooked/Models/Bistro_Aerial_B.hp_mdl",
		L"Cooked/Models/Bistro_Building_01.hp_mdl",
		L"Cooked/Models/Bistro_Building_02.hp_mdl",
		L"Cooked/Models/Bistro_Building_03.hp_mdl",
		L"Cooked/Models/Bistro_Building_04.hp_mdl",
		L"Cooked/Models/Bistro_Building_05.hp_mdl",
		L"Cooked/Models/Bistro_Building_06.hp_mdl",
		L"Cooked/Models/Bistro_Building_07.hp_mdl",
		L"Cooked/Models/Bistro_Building_08.hp_mdl",
		L"Cooked/Models/Bistro_Building_09.hp_mdl",
		L"Cooked/Models/Bistro_Building_10.hp_mdl",
		L"Cooked/Models/Bistro_Building_11.hp_mdl",
		L"Cooked/Models/Bistro_Street.hp_mdl",
		L"Cooked/Models/Bistro_Street_NormalFix.hp_mdl",
	};

	//std::vector<std::wstring> ModelPaths =
	//{
	//	L"Cooked/Models/PBRTest.hp_mdl"
	//};

	std::vector<HPModel_s> ModelAssets;
	ModelAssets.resize(ModelPaths.size());

	Concurrency::parallel_for((size_t)0u, ModelPaths.size(), [&](size_t i)
	{
		HPModel_s LoadedModel;
		if (LoadedModel.Serialize(ModelPaths[i], FileStreamMode_e::READ))
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
		VertexShader_t MeshVS = CreateVertexShader("Shaders/Mesh.hlsl");
		MeshShader_t MeshMS = CreateMeshShader("Shaders/Mesh.hlsl");
		PixelShader_t MeshPS = CreatePixelShader("Shaders/Mesh.hlsl");

		GraphicsPipelineStateDesc PsoDesc = {};
		PsoDesc.RasterizerDesc(PrimitiveTopologyType::TRIANGLE, FillMode::SOLID, CullMode::BACK)
			.DepthDesc(true, ComparisionFunc::LESS_EQUAL)
			.TargetBlendDesc({ RenderFormat::R16G16B16A16_FLOAT, RenderFormat::R16G16B16A16_FLOAT, RenderFormat::R16G16_FLOAT }, { BlendMode::None(), BlendMode::None(), BlendMode::None() }, RenderFormat::D32_FLOAT)
			.VertexShader(MeshVS)
			.PixelShader(MeshPS);

		G.MeshVSPSO = CreateGraphicsPipelineState(PsoDesc);

		PsoDesc.VertexShader(VertexShader_t::INVALID)
			.MeshShader(MeshMS);

		G.MeshMSPSO = CreateGraphicsPipelineState(PsoDesc);
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

	// Create default material
	G.DefaultMaterial.MaterialConstantBuffer = tpr::CreateConstantBuffer(&G.DefaultMaterial.Params);

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

	G.SceneColor.Init(G.ScreenWidth, G.ScreenHeight, RenderFormat::R16G16B16A16_FLOAT, L"SceneColor");
	G.SceneNormal.Init(G.ScreenWidth, G.ScreenHeight, RenderFormat::R16G16B16A16_FLOAT, L"SceneNormal");
	G.SceneRoughnessMetallic.Init(G.ScreenWidth, G.ScreenHeight, RenderFormat::R16G16_FLOAT, L"SceneRoughnessMetallic");
	G.SceneDepth.Init(G.ScreenWidth, G.ScreenHeight, RenderFormat::D32_FLOAT, RenderFormat::R32_FLOAT, L"SceneDepth");

	G.Cam.Resize(G.ScreenWidth, G.ScreenHeight);
}

void Update(float deltaSeconds)
{
	G.Cam.UpdateView(deltaSeconds);
}

void ImguiUpdate()
{
	if (ImGui::Begin("RaytracingDemo"))
	{
		ImGui::Checkbox("Use Mesh Shaders", &G.UseMeshShaders);
		ImGui::Checkbox("Show Mesh ID", &G.ShowMeshID);
		const char* DrawModeNames = "Lit\0Color\0Normal\0Roughness\0Metallic\0Depth\0Position\0Lighting\0";
		ImGui::Combo("Draw Mode", &G.DrawMode, DrawModeNames);
		if (ImGui::Button("Recompile Shaders"))
		{
			ReloadShaders();
			ReloadPipelines();
		}
		ImGui::End();
	}
}

void Render(tpr::RenderView* view, tpr::CommandListSubmissionGroup* clGroup, float deltaSeconds)
{
	struct
	{
		matrix viewProjection;
		float3 CamPos;
		uint32_t DebugMeshID;
	} ViewConsts;

	ViewConsts.viewProjection = G.Cam.GetView() * G.Cam.GetProjection();
	ViewConsts.CamPos = G.Cam.GetPosition();
	ViewConsts.DebugMeshID = G.ShowMeshID;

	DynamicBuffer_t ViewCBuf = CreateDynamicConstantBuffer(&ViewConsts);

	struct
	{
		matrix InverseProjection;
		matrix InverseView;
		uint32_t SceneColorTextureIndex;
		uint32_t SceneNormalTextureIndex;
		uint32_t SceneRoughnessMetallicTextureIndex;
		uint32_t DepthTextureIndex;
		uint32_t DrawMode;
		float3 CamPosition;
	} DeferredConsts;

	DeferredConsts.InverseProjection = InverseMatrix(G.Cam.GetProjection());
	DeferredConsts.InverseView = InverseMatrix(G.Cam.GetView());
	DeferredConsts.SceneColorTextureIndex = GetDescriptorIndex(G.SceneColor.SRV);
	DeferredConsts.SceneNormalTextureIndex = GetDescriptorIndex(G.SceneNormal.SRV);
	DeferredConsts.SceneRoughnessMetallicTextureIndex = GetDescriptorIndex(G.SceneRoughnessMetallic.SRV);
	DeferredConsts.DepthTextureIndex = GetDescriptorIndex(G.SceneDepth.SRV);
	DeferredConsts.DrawMode = G.DrawMode;
	DeferredConsts.CamPosition = G.Cam.GetPosition();

	DynamicBuffer_t DeferredCBuf = CreateDynamicConstantBuffer(&DeferredConsts);

	CommandList* cl = clGroup->CreateCommandList();

	cl->SetRootSignature();

	// Bind and clear targets
	{
		constexpr float DefaultClearCol[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		cl->ClearRenderTarget(G.SceneColor.RTV, DefaultClearCol);
		cl->ClearRenderTarget(G.SceneNormal.RTV, DefaultClearCol);
		cl->ClearRenderTarget(G.SceneRoughnessMetallic.RTV, DefaultClearCol);
		cl->ClearDepth(G.SceneDepth.DSV, 1.0f);

		RenderTargetView_t SceneTargets[] = { G.SceneColor.RTV, G.SceneNormal.RTV, G.SceneRoughnessMetallic.RTV };
		cl->SetRenderTargets(SceneTargets, ARRAYSIZE(SceneTargets), G.SceneDepth.DSV);
	}

	// Init viewport and scissor
	{
		Viewport vp{ G.ScreenWidth, G.ScreenHeight };
		cl->SetViewports(&vp, 1);
		cl->SetDefaultScissor();
	}

	// Bind draw buffers
	{
		cl->SetGraphicsRootCBV(RS_VIEW_BUF, ViewCBuf);
		cl->SetGraphicsRootDescriptorTable(RS_SRV_TABLE);
	}

	if (G.UseMeshShaders)
	{
		cl->SetPipelineState(G.MeshMSPSO);
	}
	else
	{
		cl->SetPipelineState(G.MeshVSPSO);
	}

	// Temp basic material for all meshes
	cl->SetGraphicsRootCBV(RS_MAT_BUF, G.DefaultMaterial.MaterialConstantBuffer);
	for (const RTDModel_s& Model : G.Models)
	{
		Model.Draw(cl);
	}

	// Transition for deferred pass
	{
		cl->TransitionResource(G.SceneColor.Texture, tpr::ResourceTransitionState::RENDER_TARGET, tpr::ResourceTransitionState::PIXEL_SHADER_RESOURCE);
		cl->TransitionResource(G.SceneNormal.Texture, tpr::ResourceTransitionState::RENDER_TARGET, tpr::ResourceTransitionState::PIXEL_SHADER_RESOURCE);
		cl->TransitionResource(G.SceneRoughnessMetallic.Texture, tpr::ResourceTransitionState::RENDER_TARGET, tpr::ResourceTransitionState::PIXEL_SHADER_RESOURCE);
		cl->TransitionResource(G.SceneDepth.Texture, tpr::ResourceTransitionState::DEPTH_WRITE, tpr::ResourceTransitionState::PIXEL_SHADER_RESOURCE);
	}

	// Bind back buffer target
	{
		constexpr float DefaultClearCol[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

		RenderTargetView_t backBufferRtv = view->GetCurrentBackBufferRTV();

		cl->ClearRenderTarget(backBufferRtv, DefaultClearCol);

		cl->SetRenderTargets(&backBufferRtv, 1, G.SceneDepth.DSV);
	}

	// Render deferred
	{
		cl->SetGraphicsRootCBV(RS_VIEW_BUF, DeferredCBuf);

		cl->SetPipelineState(G.DeferredPSO);

		cl->DrawInstanced(6u, 1u, 0u, 0u);
	}

	// Transition for next pass
	{
		cl->TransitionResource(G.SceneColor.Texture, tpr::ResourceTransitionState::PIXEL_SHADER_RESOURCE, tpr::ResourceTransitionState::RENDER_TARGET);
		cl->TransitionResource(G.SceneNormal.Texture, tpr::ResourceTransitionState::PIXEL_SHADER_RESOURCE, tpr::ResourceTransitionState::RENDER_TARGET);
		cl->TransitionResource(G.SceneRoughnessMetallic.Texture, tpr::ResourceTransitionState::PIXEL_SHADER_RESOURCE, tpr::ResourceTransitionState::RENDER_TARGET);
		cl->TransitionResource(G.SceneDepth.Texture, tpr::ResourceTransitionState::PIXEL_SHADER_RESOURCE, tpr::ResourceTransitionState::DEPTH_WRITE);
	}
}

void ShutdownApp()
{

}


