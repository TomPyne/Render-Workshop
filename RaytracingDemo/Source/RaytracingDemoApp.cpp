#include <Render/Render.h>
#include <SurfMath.h>
#include "imgui.h"

#include <Assets/Assets.h>
#include <Camera/FlyCamera.h>
#include <Logging/Logging.h>
#include <ModelUtils/Model.h>
#include <Materials/Materials.h>
#include <TextureUtils/TextureManager.h>

#include <ppl.h>

using namespace tpr;

struct RTDTexture_s
{
	tpr::TexturePtr Texture = {};
	tpr::ShaderResourceViewPtr SRV = {};
};

struct RTDMaterialParams_s
{
	float3 Albedo;
	float __Pad0;
	uint32_t AlbedoTextureIndex;
	uint32_t NormalTextureIndex;
	uint32_t MetallicRoughnessTextureIndex;
	float __Pad1;
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

	void Init(const ModelAsset_s* Asset);

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

struct Globals_s
{
	std::vector<RTDModel_s> Models;

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
	GraphicsPipelineStatePtr MeshVSPSO;
	GraphicsPipelineStatePtr MeshMSPSO;
	GraphicsPipelineStatePtr DeferredPSO;

	std::map<ModelAsset_s*, std::unique_ptr<RTDModel_s>> ModelMap;
	std::map<MaterialAsset_s*, std::unique_ptr<RTDMaterial_s>> MaterialMap;
	std::map<MaterialAsset_s*, std::unique_ptr<RTDTexture_s>> TextureMap;

	bool UseMeshShaders = false;
} G;

void RTDModel_s::Init(const ModelAsset_s* Asset)
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
	for (const ModelAsset_s::Mesh_s& MeshFromAsset : Asset->Meshes)
	{
		RTDMesh_s Mesh = {};
		Mesh.IndexCount = MeshFromAsset.IndexCount;
		Mesh.IndexOffset = MeshFromAsset.IndexOffset;
		Mesh.MeshletOffset = MeshFromAsset.MeshletOffset;
		Mesh.MeshletCount = MeshFromAsset.MeshletCount;


		

		Meshes.push_back(Mesh);
	}

	RTDMaterialParams_s MaterialParams = {};
	MaterialParams.Albedo = float3(1, 1, 1);
	MaterialParams.AlbedoTextureIndex = 0;
	//ModelMaterial.MaterialConstantBuffer = tpr::CreateConstantBuffer(&MaterialParams);
}

void RTDModel_s::Draw(tpr::CommandList* CL) const
{
	// Temp basic material for all meshes
	//CL->SetGraphicsRootCBV(RS_MAT_BUF, ModelMaterial.MaterialConstantBuffer);
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
	Params.DebugEnabled = true;
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
		L"Assets/Models/Bistro_Aerial_B.obj",
		//L"Assets/Models/Bistro_Aerial_C.obj",
		//L"Assets/Models/Bistro_Ashtray.obj",
		//L"Assets/Models/Bistro_Awning.obj",
		//L"Assets/Models/Bistro_Awning_02.obj",
		//L"Assets/Models/Bistro_Bollard.obj",
		//L"Assets/Models/Bistro_Building_01.obj",
		//L"Assets/Models/Bistro_Building_02.obj",
		//L"Assets/Models/Bistro_Building_03.obj",
		//L"Assets/Models/Bistro_Building_03_Mirrored.obj",
		//L"Assets/Models/Bistro_Building_04.obj",
		//L"Assets/Models/Bistro_Building_05.obj",
		//L"Assets/Models/Bistro_Building_06.obj",
		//L"Assets/Models/Bistro_Building_07.obj",
		//L"Assets/Models/Bistro_Building_08.obj",
		//L"Assets/Models/Bistro_Building_09.obj",
		//L"Assets/Models/Bistro_Building_10.obj",
		//L"Assets/Models/Bistro_Building_11.obj",
		//L"Assets/Models/Bistro_Bux_Hedge_Ball_Small.obj",
		//L"Assets/Models/Bistro_Bux_Hedge_Box_High.obj",
		//L"Assets/Models/Bistro_Bux_Hedge_Box_Long_High.obj",
		//L"Assets/Models/Bistro_Bux_Hedge_Box_Long_Low.obj",
		//L"Assets/Models/Bistro_Bux_Hedge_Box_Long_Low_Bend.obj",
		//L"Assets/Models/Bistro_Bux_Hedge_Box_Low.obj",
		//L"Assets/Models/Bistro_Bux_Hedge_Cylinder_Low.obj",
		//L"Assets/Models/Bistro_Chair_01.obj",
		//L"Assets/Models/Bistro_Chimney_01A.obj",
		//L"Assets/Models/Bistro_Chimney_01B.obj",
		//L"Assets/Models/Bistro_Chimney_02A.obj",
		//L"Assets/Models/Bistro_Chimney_02B.obj",
		//L"Assets/Models/Bistro_Chimney_02C.obj",
		//L"Assets/Models/Bistro_Doors.obj",
		//L"Assets/Models/Bistro_Electric_Box.obj",
		//L"Assets/Models/Bistro_Flowers_01A.obj",
		//L"Assets/Models/Bistro_Flowers_01B.obj",
		//L"Assets/Models/Bistro_Flowers_01C.obj",
		//L"Assets/Models/Bistro_Flowers_01D.obj",
		//L"Assets/Models/Bistro_Flowers_01E.obj",
		//L"Assets/Models/Bistro_Front_Banner.obj",
		//L"Assets/Models/Bistro_Glass.obj",
		//L"Assets/Models/Bistro_Italian_Cypress.obj",
		//L"Assets/Models/Bistro_Ivy.obj",
		//L"Assets/Models/Bistro_Lantern_Wind.obj",
		//L"Assets/Models/Bistro_Linde_Tree_Large.obj",
		//L"Assets/Models/Bistro_Liquor_Bottle_01A.obj",
		//L"Assets/Models/Bistro_Main_Balcony.obj",
		//L"Assets/Models/Bistro_Manhole.obj",
		//L"Assets/Models/Bistro_Menu_Sign_01A.obj",
		//L"Assets/Models/Bistro_Menu_Sign_01B.obj",
		//L"Assets/Models/Bistro_Napkin_Holder.obj",
		//L"Assets/Models/Bistro_Odometer.obj",
		//L"Assets/Models/Bistro_Plant_Pot.obj",
		//L"Assets/Models/Bistro_Shop_Sign_A.obj",
		//L"Assets/Models/Bistro_Shop_Sign_B.obj",
		//L"Assets/Models/Bistro_Shop_Sign_C.obj",
		//L"Assets/Models/Bistro_Shop_Sign_D.obj",
		//L"Assets/Models/Bistro_Shop_Sign_E.obj",
		//L"Assets/Models/Bistro_Sidewalk_Barrier.obj",
		//L"Assets/Models/Bistro_Spotlights.obj",
		//L"Assets/Models/Bistro_Street.obj",
		//L"Assets/Models/Bistro_Street_Light_A.obj",
		//L"Assets/Models/Bistro_Street_Light_B.obj",
		//L"Assets/Models/Bistro_Street_Light_Glass_A.obj",
		//L"Assets/Models/Bistro_Street_Light_Glass_B.obj",
		//L"Assets/Models/Bistro_Street_NormalFix.obj",
		//L"Assets/Models/Bistro_Street_Pivot.obj",
		//L"Assets/Models/Bistro_Street_Sign.obj",
		//L"Assets/Models/Bistro_String_Light_Wind.obj",
		//L"Assets/Models/Bistro_Table.obj",
		//L"Assets/Models/Bistro_Traffic_Sign.obj",
		//L"Assets/Models/Bistro_Trash_Can.obj",
		//L"Assets/Models/Bistro_Trash_Can_Open.obj",
		//L"Assets/Models/Bistro_Vespa.obj",
	};

	std::vector<ModelAsset_s*> ModelAssets;
	ModelAssets.resize(ModelPaths.size());

	Concurrency::parallel_for((size_t)0u, ModelPaths.size(), [&](size_t i)
	{
		ModelAssets[i] = LoadModel(ModelPaths[i]);
	});

	for (ModelAsset_s* ModelAsset : ModelAssets)
	{
		if (!ModelAsset)
		{
			LOGERROR("Failed to load model");
			continue;
		}

		G.ModelMap[ModelAsset] = std::make_unique<RTDModel_s>();

		G.ModelMap[ModelAsset]->Init(ModelAsset);
	}

	// Mesh PSO
	{
		VertexShader_t MeshVS = CreateVertexShader("Shaders/Mesh.hlsl");
		MeshShader_t MeshMS = CreateMeshShader("Shaders/Mesh.hlsl");
		PixelShader_t MeshPS = CreatePixelShader("Shaders/Mesh.hlsl");

		GraphicsPipelineStateDesc PsoDesc = {};
		PsoDesc.RasterizerDesc(PrimitiveTopologyType::TRIANGLE, FillMode::SOLID, CullMode::BACK)
			.DepthDesc(true, ComparisionFunc::LESS_EQUAL)
			.TargetBlendDesc({ RenderFormat::R16G16B16A16_FLOAT, RenderFormat::R16G16B16A16_FLOAT }, { BlendMode::None(), BlendMode::None()}, RenderFormat::D32_FLOAT)
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
	DepthDesc.ResourceFormat = RenderFormat::D32_FLOAT;
	DepthDesc.Height = G.ScreenHeight;
	DepthDesc.Width = G.ScreenWidth;
	DepthDesc.InitialState = ResourceTransitionState::DEPTH_WRITE;
	DepthDesc.Dimension = TextureDimension::TEX2D;
	G.DepthTexture = CreateTextureEx(DepthDesc);
	G.DepthDSV = CreateTextureDSV(G.DepthTexture, RenderFormat::D32_FLOAT, TextureDimension::TEX2D, 1u);

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
		ImGui::End();
	}
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

	for (const RTDModel_s& Model : G.Models)
	{
		Model.Draw(cl);
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
		cl->SetGraphicsRootCBV(RS_VIEW_BUF, DeferredCBuf);

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


