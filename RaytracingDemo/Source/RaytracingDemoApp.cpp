#include <Render/Render.h>
#include <Render/Raytracing.h>
#include <SurfMath.h>
#include "imgui.h"

#include <Assets/Assets.h>
#include <Camera/FlyCamera.h>
#include <FileUtils/FileStream.h>
#include <Logging/Logging.h>
#include <RenderUtils/RenderGraph/RenderGraph.h>

#include <HPModel.h>
#include <HPWfMtlLib.h>
#include <HPTexture.h>

#include <ppl.h>

using namespace rl;

static const std::wstring s_AssetDirectory = L"Cooked/";

struct RTDTexture_s
{
	rl::TexturePtr Texture = {};
	rl::ShaderResourceViewPtr SRV = {};

	bool Init(const HPTexture_s& Asset);
};

struct RTDMaterialParams_s
{
	uint32_t AlbedoTextureIndex = 0;
	uint32_t NormalTextureIndex = 0;
	uint32_t RoughnessMetallicTextureIndex = 0;
	float __Pad;
};

struct RTDMaterial_s
{
	RTDMaterialParams_s Params;

	std::shared_ptr<RTDTexture_s> AlbedoTexture = nullptr;
	std::shared_ptr<RTDTexture_s> NormalTexture = nullptr;
	std::shared_ptr<RTDTexture_s> RoughnessMetallicTexture = nullptr;

	rl::ConstantBuffer_t MaterialConstantBuffer = {};

	bool Init(const HPWfMtlLib_s::Material_s& Asset);
};

struct RTDBuffer_s
{
	rl::StructuredBufferPtr StructuredBuffer = {};
	rl::ShaderResourceViewPtr BufferSRV = {};

	template<typename T>
	void Init(const T* const Data, size_t Count)
	{
		if (Count)
		{
			CHECK(Data != nullptr);
			StructuredBuffer = rl::CreateStructuredBuffer(Data, Count);
			CHECK(StructuredBuffer);
			BufferSRV = rl::CreateStructuredBufferSRV(StructuredBuffer, 0u, static_cast<uint32_t>(Count), static_cast<uint32_t>(sizeof(T)));
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

	RaytracingGeometryPtr RaytracingGeometry = {};

	std::shared_ptr<RTDMaterial_s> Material = nullptr;
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

	rl::ConstantBufferPtr ModelConstantBuffer;

	std::vector<RTDMesh_s> Meshes;

	bool Init(const HPModel_s* Asset);

	void Draw(rl::CommandList* CL) const;
};

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
	ComputePipelineStatePtr UAVClearF1PSO;
	ComputePipelineStatePtr UAVClearF2PSO;
	ComputePipelineStatePtr ShadowDenoisePSO;
	RaytracingPipelineStatePtr RTPSO;

	// RT Root Signature
	RootSignaturePtr RTRootSignature;

	// RG
	RenderGraphResourcePool_s RenderGraphResourcePool;

	std::map<std::wstring, std::shared_ptr<RTDModel_s>> ModelMap;
	std::map<std::wstring, std::shared_ptr<RTDMaterial_s>> MaterialMap;
	std::map<std::wstring, std::shared_ptr<RTDTexture_s>> TextureMap;

	RTDMaterial_s DefaultMaterial = {};

	RaytracingScenePtr RaytracingScene = {};
	RaytracingShaderTablePtr RaytracingShaderTable = {};

	bool UseMeshShaders = false;
	bool ShowMeshID = false;
	bool ShowShadows = true;
	int32_t DrawMode = 0;

	float SunYaw = 0.0f;
	float SunPitch = 1.0f;
	float SunSoftAngle = 0.01f;

	float ElapsedTime = 0.0f;
} G;

float3 GetSunDirection()
{
	const float CosTheta = cosf(G.SunPitch);
	const float SinTheta = sinf(G.SunPitch);
	const float CosPhi = cosf(G.SunYaw);
	const float SinPhi = sinf(G.SunYaw);

	return Normalize(float3(CosPhi * CosTheta, SinTheta, SinPhi * CosTheta));
}

bool RTDTexture_s::Init(const HPTexture_s& Asset)
{
	rl::TextureCreateDescEx TexDesc = {};
	TexDesc.DebugName = Asset.SourcePath;
	TexDesc.Width = Asset.Width;
	TexDesc.Height = Asset.Height;
	TexDesc.DepthOrArraySize = 1u;
	TexDesc.MipCount = static_cast<uint32_t>(Asset.Mips.size());
	TexDesc.Dimension = TextureDimension::TEX2D;
	TexDesc.Flags = rl::RenderResourceFlags::SRV;
	TexDesc.ResourceFormat = Asset.Format;

	std::vector<MipData> Data;
	Data.resize(Asset.Mips.size());
	for (size_t MipIt = 0; MipIt < Asset.Mips.size(); MipIt++)
	{
		Data[MipIt].RowPitch = Asset.Mips[MipIt].RowPitch;
		Data[MipIt].SlicePitch = Asset.Mips[MipIt].SlicePitch;
		Data[MipIt].Data = Asset.Data.data() + Asset.Mips[MipIt].Offset;
	}

	TexDesc.Data = Data.data();

	Texture = rl::CreateTextureEx(TexDesc);
	SRV = rl::CreateTextureSRV(Texture);

	return true;
}

bool RTDMaterial_s::Init(const HPWfMtlLib_s::Material_s& Asset)
{
	auto LoadTexture = [](const std::wstring& TexPath) -> std::shared_ptr<RTDTexture_s>
	{
		if (TexPath.empty())
			return nullptr;

		HPTexture_s TextureAsset;
		if (!TextureAsset.Serialize(s_AssetDirectory + TexPath, FileStreamMode_e::READ))
		{
			LOGERROR("Failed to load texture asset");
			return nullptr;
		}

		auto TexIt = G.TextureMap.find(TexPath);
		if (TexIt != G.TextureMap.end())
		{
			return TexIt->second;
		}

		std::shared_ptr<RTDTexture_s> NewTexture = std::make_shared<RTDTexture_s>();
		if (NewTexture->Init(TextureAsset))
		{
			G.TextureMap[TexPath] = NewTexture;
		}
		else
		{
			G.TextureMap[TexPath] = nullptr;
		}

		return G.TextureMap[TexPath];
	};

	AlbedoTexture = LoadTexture(Asset.DiffuseTexture);
	RoughnessMetallicTexture = LoadTexture(Asset.SpecularTexture);
	NormalTexture = LoadTexture(Asset.NormalTexture);

	Params.AlbedoTextureIndex = AlbedoTexture ? rl::GetDescriptorIndex(AlbedoTexture->SRV) : 0;
	Params.RoughnessMetallicTextureIndex = RoughnessMetallicTexture ? rl::GetDescriptorIndex(RoughnessMetallicTexture->SRV) : 0;
	Params.NormalTextureIndex = NormalTexture ? rl::GetDescriptorIndex(NormalTexture->SRV) : 0;

	MaterialConstantBuffer = rl::CreateConstantBuffer(&Params);

	return true;
}

bool RTDModel_s::Init(const HPModel_s* Asset)
{
	CHECK(Asset != nullptr);

	CHECK(!Asset->Positions.empty());
	PositionBuffer.Init(Asset->Positions.data(), Asset->Positions.size());

	CHECK(!Asset->Indices.empty());
	if (Asset->IndexFormat == rl::RenderFormat::R32_UINT)
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
	MeshConstants.PositionBufSRVIndex = rl::GetDescriptorIndex(PositionBuffer.BufferSRV);
	MeshConstants.NormalBufSRVIndex = rl::GetDescriptorIndex(NormalBuffer.BufferSRV);
	MeshConstants.TangentBufSRVIndex = rl::GetDescriptorIndex(TangentBuffer.BufferSRV);
	MeshConstants.BitangentBufSRVIndex = rl::GetDescriptorIndex(BitangentBuffer.BufferSRV);
	MeshConstants.TexcoordBufSRVIndex = rl::GetDescriptorIndex(TexcoordBuffer.BufferSRV);
	MeshConstants.IndexBufSRVIndex = rl::GetDescriptorIndex(IndexBuffer.BufferSRV);

	MeshConstants.MeshletBufSRVIndex = rl::GetDescriptorIndex(MeshletBuffer.BufferSRV);
	MeshConstants.UniqueVertexIndexBufSRVIndex = rl::GetDescriptorIndex(UniqueVertexIndexBuffer.BufferSRV);
	MeshConstants.PrimitiveIndexBufSRVIndex = rl::GetDescriptorIndex(PrimitiveIndexBuffer.BufferSRV);

	ModelConstantBuffer = rl::CreateConstantBuffer(&MeshConstants);

	HPWfMtlLib_s MaterialLib;
	if (Asset->MaterialLibPath.empty())
	{
		LOGERROR("No material lib provided for %S, default materials not handled", Asset->SourcePath);
		return false;
	}
	if (!MaterialLib.Serialize(s_AssetDirectory + Asset->MaterialLibPath, FileStreamMode_e::READ))
	{
		LOGERROR("Material lib % failed to load for model %S, default materials not handled", Asset->SourcePath);
		return false;
	}

	rl::RaytracingGeometryDesc RTDesc = {};
	RTDesc.StructuredVertexBuffer = PositionBuffer.StructuredBuffer;
	RTDesc.VertexFormat = RenderFormat::R32G32B32_FLOAT;
	RTDesc.VertexCount = static_cast<uint32_t>(Asset->Positions.size());
	RTDesc.VertexStride = static_cast<uint32_t>(sizeof(float3));
	RTDesc.StructuredIndexBuffer = IndexBuffer.StructuredBuffer;
	RTDesc.IndexFormat = Asset->IndexFormat;

	Meshes.reserve(Asset->Meshes.size());
	for (const HPModel_s::Mesh_s& MeshFromAsset : Asset->Meshes)
	{
		RTDMesh_s Mesh = {};
		Mesh.IndexCount = MeshFromAsset.IndexCount;
		Mesh.IndexOffset = MeshFromAsset.IndexOffset;
		Mesh.MeshletOffset = MeshFromAsset.MeshletOffset;
		Mesh.MeshletCount = MeshFromAsset.MeshletCount;

		RTDesc.IndexCount = MeshFromAsset.IndexCount;
		RTDesc.IndexOffset = MeshFromAsset.IndexOffset;

		{
			Mesh.RaytracingGeometry = CreateRaytracingGeometry(RTDesc);

			rl::AddRaytracingGeometryToScene(Mesh.RaytracingGeometry, G.RaytracingScene);
		}

		std::wstring MaterialKey = Asset->MaterialLibPath + MeshFromAsset.LibMaterialName;
		auto It = G.MaterialMap.find(MaterialKey);
		if (It != G.MaterialMap.end())
		{
			Mesh.Material = It->second;
		}
		else
		{
			std::shared_ptr<RTDMaterial_s> LoadedMaterial = nullptr;
			for (const HPWfMtlLib_s::Material_s& LibMaterial : MaterialLib.Materials)
			{
				if (LibMaterial.Name == MeshFromAsset.LibMaterialName)
				{
					std::shared_ptr<RTDMaterial_s> NewMaterial = std::make_shared<RTDMaterial_s>();
					if (NewMaterial->Init(LibMaterial))
					{
						LoadedMaterial = NewMaterial;
					}
					else
					{
						LOGERROR("Failed to create material %S", MaterialKey.c_str());
					}					

					break;
				}
			}

			if (LoadedMaterial == nullptr)
			{
				LOGERROR("Failed to find material %S for %S", MeshFromAsset.LibMaterialName.c_str(), Asset->SourcePath.c_str());
			}

			G.MaterialMap[MaterialKey] = LoadedMaterial;
			Mesh.Material = LoadedMaterial;
		}

		Meshes.push_back(Mesh);
	}

	return true;
}

void RTDModel_s::Draw(rl::CommandList* CL) const
{
	CL->SetGraphicsRootCBV(GlobalRootSigSlots::RS_MODEL_BUF, ModelConstantBuffer);

	if (G.UseMeshShaders)
	{
		for (const RTDMesh_s& Mesh : Meshes)
		{
			CL->SetGraphicsRootValue(GlobalRootSigSlots::RS_DRAWCONSTANTS, DCS_MESHLET_OFFSET, Mesh.MeshletOffset);

			CL->SetGraphicsRootCBV(GlobalRootSigSlots::RS_MAT_BUF, Mesh.Material ? Mesh.Material->MaterialConstantBuffer : G.DefaultMaterial.MaterialConstantBuffer);

			CL->DispatchMesh(Mesh.MeshletCount, 1u, 1u);
		}
	}
	else
	{
		for (const RTDMesh_s& Mesh : Meshes)
		{
			CL->SetGraphicsRootValue(GlobalRootSigSlots::RS_DRAWCONSTANTS, DCS_INDEX_OFFSET, Mesh.IndexOffset);

			CL->SetGraphicsRootCBV(GlobalRootSigSlots::RS_MAT_BUF, Mesh.Material ? Mesh.Material->MaterialConstantBuffer : G.DefaultMaterial.MaterialConstantBuffer);

			CL->DrawInstanced(Mesh.IndexCount, 1u, 0u, 0u);
		}
	}
}

rl::RenderInitParams GetAppRenderParams()
{
	rl::RenderInitParams Params;
#ifdef _DEBUG
	Params.DebugEnabled = true;
#else
	Params.DebugEnabled = false;
#endif

	Params.RootSigDesc.Flags = RootSignatureFlags::NONE;
	Params.RootSigDesc.Slots.resize(GlobalRootSigSlots::RS_COUNT);
	Params.RootSigDesc.Slots[GlobalRootSigSlots::RS_DRAWCONSTANTS] = RootSignatureSlot::ConstantsSlot(DCS_COUNT, 0);
	Params.RootSigDesc.Slots[GlobalRootSigSlots::RS_VIEW_BUF] = RootSignatureSlot::CBVSlot(1, 0);
	Params.RootSigDesc.Slots[GlobalRootSigSlots::RS_MODEL_BUF] = RootSignatureSlot::CBVSlot(2, 0);
	Params.RootSigDesc.Slots[GlobalRootSigSlots::RS_MAT_BUF] = RootSignatureSlot::CBVSlot(3, 0);
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

	G.RaytracingScene = rl::CreateRaytracingScene();

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
		VertexShader_t MeshVS = CreateVertexShader("Shaders/Mesh.hlsl");
		MeshShader_t MeshMS = CreateMeshShader("Shaders/Mesh.hlsl");
		PixelShader_t MeshPS = CreatePixelShader("Shaders/Mesh.hlsl");

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

		PsoDesc.Cs = CreateComputeShader("Shaders/ClearUAV.hlsl", { "F1" });
		PsoDesc.DebugName = L"ClearUAVF1";		
		G.UAVClearF1PSO = CreateComputePipelineState(PsoDesc);

		PsoDesc.Cs = CreateComputeShader("Shaders/ClearUAV.hlsl", { "F2" });
		PsoDesc.DebugName = L"ClearUAVF2";
		G.UAVClearF2PSO = CreateComputePipelineState(PsoDesc);
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

		PsoDesc.DebugName = L"DeferredPSO";

		G.DeferredPSO = CreateGraphicsPipelineState(PsoDesc);
	}

	// Denoiser PSO
	{
		ComputeShader_t ShadowDenoiserCS = CreateComputeShader("Shaders/ShadowDenoiser.hlsl");
		ComputePipelineStateDesc PsoDesc = {};
		PsoDesc.Cs = ShadowDenoiserCS;
		PsoDesc.DebugName = L"ShadowDenoiser";

		G.ShadowDenoisePSO = CreateComputePipelineState(PsoDesc);
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
		RTDesc.RayGenShader = rl::CreateRayGenShader("Shaders/RTShadows.hlsl");
		RTDesc.MissShader = rl::CreateMissShader("Shaders/RTShadows.hlsl");
		RTDesc.DebugName = L"RTShadow";
		RTDesc.RootSig = G.RTRootSignature;
		G.RTPSO = CreateRaytracingPipelineState(RTDesc);

		BuildRaytracingScene(G.RaytracingScene);

		RaytracingShaderTableLayout ShaderTableLayout;
		ShaderTableLayout.RayGenShader = RTDesc.RayGenShader;
		ShaderTableLayout.MissShader = RTDesc.MissShader;
		G.RaytracingShaderTable = CreateRaytracingShaderTable(G.RTPSO, ShaderTableLayout);
	}

	// Create default material
	G.DefaultMaterial.MaterialConstantBuffer = rl::CreateConstantBuffer(&G.DefaultMaterial.Params);

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

	G.SceneConfidenceHistoryRGTexture = CreateRenderGraphTexture(G.ScreenWidth, G.ScreenHeight, RenderFormat::R8_UNORM, RenderGraphResourceAccessType_e::UAV | RenderGraphResourceAccessType_e::SRV, L"SceneConfidenceHistory");
	G.SceneShadowHistoryRGTexture = CreateRenderGraphTexture(G.ScreenWidth, G.ScreenHeight, RenderFormat::R8_UNORM, RenderGraphResourceAccessType_e::UAV | RenderGraphResourceAccessType_e::SRV, L"SceneShadowHistory");
	G.SceneDepthHistoryRGTexture = CreateRenderGraphTexture(G.ScreenWidth, G.ScreenHeight, RenderFormat::R32_FLOAT, RenderGraphResourceAccessType_e::UAV | RenderGraphResourceAccessType_e::SRV, L"SceneDepthHistory");

	G.Cam.Resize(G.ScreenWidth, G.ScreenHeight);
}

void Update(float deltaSeconds)
{
	G.ElapsedTime += deltaSeconds;

	G.Cam.UpdateView(deltaSeconds);
}

void ImguiUpdate()
{
	if (ImGui::Begin("RaytracingDemo"))
	{
		ImGui::Checkbox("Use Mesh Shaders", &G.UseMeshShaders);
		ImGui::Checkbox("Show Mesh ID", &G.ShowMeshID);
		ImGui::Checkbox("Show Shadows", &G.ShowShadows);
		const char* DrawModeNames = "Lit\0Color\0Normal\0Roughness\0Metallic\0Depth\0Position\0Lighting\0RTShadows\0Velocity\0Disocclusion\0";
		ImGui::Combo("Draw Mode", &G.DrawMode, DrawModeNames);
		ImGui::Separator();
		if (ImGui::SliderAngle("Sun Yaw", &G.SunYaw, 0.0f, 360.0f))
		{
			G.FramesSinceMove = 0;
		}
		if (ImGui::SliderAngle("Sun Pitch", &G.SunPitch, 0.0f, 90.0f))
		{
			G.FramesSinceMove = 0;
		}
		if (ImGui::SliderAngle("Sun Soft Angle", &G.SunSoftAngle, 0.0f, 5.0f))
		{
			G.FramesSinceMove = 0;
		}
		ImGui::Separator();
		if (ImGui::Button("Recompile Shaders"))
		{
			ReloadShaders();
			ReloadPipelines();
		}
	}
	ImGui::End();
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

	struct
	{
		matrix ViewProjection;
		matrix PrevviewProjection;
		float3 CamPos;
		uint32_t DebugMeshID;
	} ViewConsts;

	ViewConsts.ViewProjection = ViewProjection;
	ViewConsts.PrevviewProjection = G.PrevViewProjection;
	ViewConsts.CamPos = G.Cam.GetPosition();
	ViewConsts.DebugMeshID = G.ShowMeshID;

	DynamicBuffer_t ViewCBuf = CreateDynamicConstantBuffer(&ViewConsts);

	RenderGraphBuilder_s RGBuilder(G.RenderGraphResourcePool);

	// Mesh draw pass
	RenderGraphResourceHandle_t SceneColorTexture = RGBuilder.CreateTexture(G.ScreenWidth, G.ScreenHeight, RenderFormat::R16G16B16A16_FLOAT, RenderGraphResourceAccessType_e::RTV | RenderGraphResourceAccessType_e::SRV, L"SceneColorTexture");
	RenderGraphResourceHandle_t SceneNormalTexture = RGBuilder.CreateTexture(G.ScreenWidth, G.ScreenHeight, RenderFormat::R16G16B16A16_FLOAT, RenderGraphResourceAccessType_e::RTV | RenderGraphResourceAccessType_e::SRV, L"SceneNormalTexture");
	RenderGraphResourceHandle_t SceneRoughnessMetallicTexture = RGBuilder.CreateTexture(G.ScreenWidth, G.ScreenHeight, RenderFormat::R16G16_FLOAT, RenderGraphResourceAccessType_e::RTV | RenderGraphResourceAccessType_e::SRV, L"SceneRoughnessMetallicTexture");
	RenderGraphResourceHandle_t SceneVelocityTexture = RGBuilder.CreateTexture(G.ScreenWidth, G.ScreenHeight, RenderFormat::R16G16_FLOAT, RenderGraphResourceAccessType_e::RTV | RenderGraphResourceAccessType_e::SRV, L"SceneVelocityTexture");
	RenderGraphResourceHandle_t SceneDepthTexture = RGBuilder.CreateTexture(G.ScreenWidth, G.ScreenHeight, RenderFormat::R32_FLOAT, RenderGraphResourceAccessType_e::DSV | RenderGraphResourceAccessType_e::SRV, L"SceneDepthTexture");

	RenderGraphPass_s& MeshDrawPass = RGBuilder.AddPass(RenderGraphPassType_e::GRAPHICS, L"Mesh Pass")
	.AccessResource(SceneColorTexture, RenderGraphResourceAccessType_e::RTV, RenderGraphLoadOp_e::CLEAR)
	.AccessResource(SceneNormalTexture, RenderGraphResourceAccessType_e::RTV, RenderGraphLoadOp_e::CLEAR)
	.AccessResource(SceneRoughnessMetallicTexture, RenderGraphResourceAccessType_e::RTV, RenderGraphLoadOp_e::CLEAR)
	.AccessResource(SceneVelocityTexture, RenderGraphResourceAccessType_e::RTV, RenderGraphLoadOp_e::CLEAR)
	.AccessResource(SceneDepthTexture, RenderGraphResourceAccessType_e::DSV, RenderGraphLoadOp_e::CLEAR)
	.SetExecuteCallback([=](RenderGraph_s& RG, rl::CommandList* CL)
	{
		CL->SetRootSignature();

		rl::RenderTargetView_t SceneRTVs[] = 
		{ 
			RG.GetRTV(SceneColorTexture),
			RG.GetRTV(SceneNormalTexture),
			RG.GetRTV(SceneRoughnessMetallicTexture),
			RG.GetRTV(SceneVelocityTexture)
		};
		rl::DepthStencilView_t SceneDSV = RG.GetDSV(SceneDepthTexture);
		CL->SetRenderTargets(SceneRTVs, ARRAYSIZE(SceneRTVs), SceneDSV); // TODO: this should be set by the graph

		Viewport vp{ G.ScreenWidth, G.ScreenHeight };
		CL->SetViewports(&vp, 1);
		CL->SetDefaultScissor(); // Could also be captured by the command context

		CL->SetGraphicsRootCBV(GlobalRootSigSlots::RS_VIEW_BUF, ViewCBuf);
		CL->SetGraphicsRootDescriptorTable(GlobalRootSigSlots::RS_SRV_TABLE); // Root sig stuff is trickier

		CL->SetPipelineState(G.UseMeshShaders ? G.MeshMSPSO : G.MeshVSPSO);

		for (const RTDModel_s& Model : G.Models)
		{
			Model.Draw(CL);
		}
	});

	// Draw RT shadows or clear RT shadows

	RenderGraphResourceHandle_t ShadowTexture = RGBuilder.CreateTexture(G.ScreenWidth, G.ScreenHeight, RenderFormat::R8_UNORM, RenderGraphResourceAccessType_e::UAV | RenderGraphResourceAccessType_e::SRV, L"CurrentFrameShadow");
	RenderGraphResourceHandle_t ConfidenceTexture = RGBuilder.CreateTexture(G.ScreenWidth, G.ScreenHeight, RenderFormat::R8_UNORM, RenderGraphResourceAccessType_e::UAV | RenderGraphResourceAccessType_e::SRV, L"SceneConfidence");

	if (G.ShowShadows)
	{
		RenderGraphPass_s& RTShadowsPass = RGBuilder.AddPass(RenderGraphPassType_e::RAYTRACING, L"RT Shadows")
		.AccessResource(SceneDepthTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
		.AccessResource(ShadowTexture, RenderGraphResourceAccessType_e::UAV, RenderGraphLoadOp_e::DONT_CARE)
		.SetExecuteCallback([=](RenderGraph_s& RG, rl::CommandList* CL)
		{
			struct RayUniforms_s
			{
				matrix CamToWorld;

				float3 SunDirection;
				float SunSoftAngle;;

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

			CL->SetComputeRootSignature(G.RTRootSignature);

			CL->SetComputeRootDescriptorTable(RTRootSigSlots::RS_UAV_TABLE);
			CL->SetComputeRootDescriptorTable(RTRootSigSlots::RS_SRV_TABLE);

			CL->SetPipelineState(G.RTPSO);

			CL->SetComputeRootCBV(RTRootSigSlots::RS_CONSTANTS, RayCBuf);
			CL->SetComputeRootSRV(RTRootSigSlots::RS_RAYTRACING_SCENE, G.RaytracingScene);

			CL->DispatchRays(G.RaytracingShaderTable, G.ScreenWidth, G.ScreenHeight, 1);
		});

		RenderGraphResourceHandle_t ShadowHistoryTexture = RGBuilder.InjectTexture(G.SceneShadowHistoryRGTexture, L"PrevFrameShadow");
		RenderGraphResourceHandle_t DepthHistoryTexture = RGBuilder.InjectTexture(G.SceneDepthHistoryRGTexture, L"PrevFrameDepth");
		RenderGraphResourceHandle_t ConfidenceHistoryTexture = RGBuilder.InjectTexture(G.SceneConfidenceHistoryRGTexture, L"PrevFrameConfidence");

		RenderGraphPass_s& RTShadowsDenoisePass = RGBuilder.AddPass(RenderGraphPassType_e::COMPUTE, L"RT Shadows Denoise")
		.AccessResource(SceneDepthTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
		.AccessResource(DepthHistoryTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
		.AccessResource(ShadowTexture, RenderGraphResourceAccessType_e::UAV, RenderGraphLoadOp_e::LOAD)
		.AccessResource(ShadowHistoryTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
		.AccessResource(ConfidenceTexture, RenderGraphResourceAccessType_e::UAV, RenderGraphLoadOp_e::DONT_CARE)
		.AccessResource(ConfidenceHistoryTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
		.AccessResource(SceneVelocityTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
		.SetExecuteCallback([=](RenderGraph_s& RG, rl::CommandList* CL)
		{
			struct DenoiseUniforms_s
			{
				matrix CamToWorld;

				matrix PrevCamToWorld;

				matrix ClipToPrevClip;

				uint32_t DepthTextureIndex;
				uint32_t PrevFrameDepthTextureIndex;
				uint32_t ShadowTextureIndex;
				uint32_t PrevFrameShadowTextureIndex;

				uint32_t ConfidenceTextureIndex;
				uint32_t PrevConfidenceTextureIndex;
				uint32_t VelocityTextureIndex;
				uint32_t DebugTextureIndex;

				float2 ViewportSizeRcp;
				uint32_t ViewportWidth;
				uint32_t ViewportHeight;
			} DenoiseUniforms;

			DenoiseUniforms.CamToWorld = InverseMatrix(ViewProjection);
			DenoiseUniforms.PrevCamToWorld = InverseMatrix(G.PrevViewProjection);
			DenoiseUniforms.ClipToPrevClip = DenoiseUniforms.PrevCamToWorld * G.PrevViewProjection;
			DenoiseUniforms.DepthTextureIndex = GetDescriptorIndex(RG.GetSRV(SceneDepthTexture));
			DenoiseUniforms.PrevFrameDepthTextureIndex = GetDescriptorIndex(RG.GetSRV(DepthHistoryTexture));
			DenoiseUniforms.ShadowTextureIndex = GetDescriptorIndex(RG.GetUAV(ShadowTexture));
			DenoiseUniforms.PrevFrameShadowTextureIndex = GetDescriptorIndex(RG.GetSRV(ShadowHistoryTexture));
			DenoiseUniforms.ConfidenceTextureIndex = GetDescriptorIndex(RG.GetUAV(ConfidenceTexture));
			DenoiseUniforms.PrevConfidenceTextureIndex = GetDescriptorIndex(RG.GetSRV(ConfidenceHistoryTexture));
			DenoiseUniforms.VelocityTextureIndex = GetDescriptorIndex(RG.GetSRV(SceneVelocityTexture));
			DenoiseUniforms.DebugTextureIndex = 0;
			DenoiseUniforms.ViewportSizeRcp = float2(1.0f / (float)G.ScreenWidth, 1.0f / (float)G.ScreenHeight);
			DenoiseUniforms.ViewportWidth = G.ScreenWidth;
			DenoiseUniforms.ViewportHeight = G.ScreenHeight;

			DynamicBuffer_t DenoiseCBuf = CreateDynamicConstantBuffer(&DenoiseUniforms);

			CL->SetRootSignature();

			CL->SetComputeRootDescriptorTable(GlobalRootSigSlots::RS_UAV_TABLE);
			CL->SetComputeRootDescriptorTable(GlobalRootSigSlots::RS_SRV_TABLE);

			CL->SetPipelineState(G.ShadowDenoisePSO);

			CL->SetComputeRootCBV(GlobalRootSigSlots::RS_VIEW_BUF, DenoiseCBuf);

			CL->Dispatch(DivideRoundUp(G.ScreenWidth, 8u), DivideRoundUp(G.ScreenHeight, 8u), 1u);
		});

		RGBuilder.QueueTextureCopy(ShadowHistoryTexture, ShadowTexture);
		RGBuilder.QueueTextureCopy(DepthHistoryTexture, SceneDepthTexture);
		RGBuilder.QueueTextureCopy(ConfidenceHistoryTexture, ConfidenceTexture);
	}
	else
	{
		RenderGraphPass_s& RTShadowsPass = RGBuilder.AddPass(RenderGraphPassType_e::COMPUTE, L"Clear Shadows")
		.AccessResource(ShadowTexture, RenderGraphResourceAccessType_e::UAV, RenderGraphLoadOp_e::DONT_CARE)
		.SetExecuteCallback([=](RenderGraph_s& RG, rl::CommandList* CL)
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

			CL->SetRootSignature();

			CL->SetComputeRootDescriptorTable(GlobalRootSigSlots::RS_UAV_TABLE);

			CL->SetPipelineState(G.UAVClearF1PSO);

			CL->SetComputeRootCBV(GlobalRootSigSlots::RS_VIEW_BUF, ClearBuf);

			CL->Dispatch(DivideRoundUp(G.ScreenWidth, 8u), DivideRoundUp(G.ScreenHeight, 8u), 1u);
		});
	}

	RenderGraphResourceHandle_t BackBufferTexture = RGBuilder.RefBackBufferTexture(View->GetCurrentBackBufferTexture(), View->GetCurrentBackBufferRTV(), rl::ResourceTransitionState::RENDER_TARGET);

	// Deferred
	RenderGraphPass_s& DeferredPass = RGBuilder.AddPass(RenderGraphPassType_e::GRAPHICS, L"Deferred Pass")
	.AccessResource(SceneColorTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(SceneNormalTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(SceneRoughnessMetallicTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(SceneDepthTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(ShadowTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(SceneVelocityTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(ConfidenceTexture, RenderGraphResourceAccessType_e::SRV, RenderGraphLoadOp_e::LOAD)
	.AccessResource(BackBufferTexture, RenderGraphResourceAccessType_e::RTV, RenderGraphLoadOp_e::DONT_CARE)
	.SetExecuteCallback([=](RenderGraph_s& RG, rl::CommandList* CL)
	{
		struct
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
		} DeferredConsts;

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

		DynamicBuffer_t DeferredCBuf = CreateDynamicConstantBuffer(&DeferredConsts);

		CL->SetRootSignature();

		rl::RenderTargetView_t BackBufferRTV = RG.GetRTV(BackBufferTexture);

		CL->SetRenderTargets(&BackBufferRTV, 1, {}); // TODO: this should be set by the graph

		Viewport vp{ G.ScreenWidth, G.ScreenHeight };
		CL->SetViewports(&vp, 1);
		CL->SetDefaultScissor(); // Could also be captured by the command context

		CL->SetGraphicsRootDescriptorTable(GlobalRootSigSlots::RS_SRV_TABLE);
		CL->SetGraphicsRootCBV(GlobalRootSigSlots::RS_VIEW_BUF, DeferredCBuf);

		CL->SetPipelineState(G.DeferredPSO);

		CL->DrawInstanced(6u, 1u, 0u, 0u);
	});

	RenderGraph_s Graph = RGBuilder.Build();

	Graph.Execute(clGroup->CreateCommandList());

	G.PrevViewProjection = ViewProjection;
}

void ShutdownApp()
{

}
