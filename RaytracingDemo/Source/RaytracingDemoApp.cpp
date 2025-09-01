#include <Render/Render.h>
#include <Render/Raytracing.h>
#include <SurfMath.h>
#include "imgui.h"

#include <Assets/Assets.h>
#include <Camera/FlyCamera.h>
#include <FileUtils/FileStream.h>
#include <Logging/Logging.h>

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

struct SceneTarget_s
{
	TexturePtr Texture = {};
	RenderTargetViewPtr RTV = {};
	ShaderResourceViewPtr SRV = {};
	UnorderedAccessViewPtr UAV = {};
	DepthStencilViewPtr DSV = {};
	ResourceTransitionState CurrentState = ResourceTransitionState::COMMON;	

	void Init(uint32_t Width, uint32_t Height, RenderFormat Format, ResourceTransitionState InitialState, bool IsRenderTarget, bool IsComputeTarget, bool IsShaderResource, const wchar_t* DebugName)
	{
		if (!IsRenderTarget && !IsComputeTarget && !IsShaderResource)
		{
			LOGERROR("Scene Target [%S] has unknown usage, at least one of IsRenderTarget, IsComputeTarget or IsShaderResource must be true", DebugName ? DebugName : L"UnknownSceneTarget");
		}

		TextureCreateDescEx Desc = {};
		Desc.DebugName = DebugName ? DebugName : L"UnknownSceneTarget";
		if(IsRenderTarget)
			Desc.Flags |= RenderResourceFlags::RTV;

		if(IsComputeTarget)
			Desc.Flags |= RenderResourceFlags::UAV;

		if (IsShaderResource)
			Desc.Flags |= RenderResourceFlags::SRV;

		Desc.ResourceFormat = Format;
		Desc.Height = Height;
		Desc.Width = Width;
		Desc.InitialState = InitialState;
		Desc.Dimension = TextureDimension::TEX2D;
		Texture = CreateTextureEx(Desc);

		if(IsRenderTarget)
			RTV = CreateTextureRTV(Texture, Format, TextureDimension::TEX2D, 1u);

		if(IsShaderResource)
			SRV = CreateTextureSRV(Texture, Format, TextureDimension::TEX2D, 1u, 1u);

		if(IsComputeTarget)
			UAV = CreateTextureUAV(Texture, Format, TextureDimension::TEX2D, 1u);

		CurrentState = InitialState;
		if (!Texture || (IsRenderTarget && !RTV) || (IsShaderResource && !SRV) || (IsComputeTarget && !UAV))
		{
			LOGERROR("Scene Target [%S] failed to intialize", Desc.DebugName.c_str());
		}
	}

	void InitAsRenderTarget(uint32_t Width, uint32_t Height, RenderFormat Format, const wchar_t* DebugName)
	{
		Init(Width, Height, Format, ResourceTransitionState::RENDER_TARGET, true, true, true, DebugName);
	}

	void InitAsCompute(uint32_t Width, uint32_t Height, RenderFormat Format, const wchar_t* DebugName)
	{
		Init(Width, Height, Format, ResourceTransitionState::UNORDERED_ACCESS, false, true, true, DebugName);
	}

	void InitAsDepth(uint32_t Width, uint32_t Height, RenderFormat DepthFormat, RenderFormat SRVFormat, const wchar_t* DebugName)
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
		CurrentState = ResourceTransitionState::DEPTH_WRITE;
		if (!Texture || !DSV || !SRV)
		{
			LOGERROR("Scene Target [%S] failed to intialize", Desc.DebugName.c_str());
		}
	}

	void Transition(CommandList* Cl, ResourceTransitionState NewState)
	{
		if (CurrentState != NewState)
		{
			Cl->TransitionResource(Texture, CurrentState, NewState);
			CurrentState = NewState;
		}
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
	SceneTarget_s SceneVelocity = {};
	SceneTarget_s SceneShadow0 = {};
	SceneTarget_s SceneShadow1 = {};
	SceneTarget_s SceneDepth0 = {};
	SceneTarget_s SceneDepth1 = {};
	SceneTarget_s SceneConfidence = {};

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

		G.MeshVSPSO = CreateGraphicsPipelineState(PsoDesc);

		PsoDesc.VertexShader(VertexShader_t::INVALID)
			.MeshShader(MeshMS);

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

	G.SceneColor.InitAsRenderTarget(G.ScreenWidth, G.ScreenHeight, RenderFormat::R16G16B16A16_FLOAT, L"SceneColor");
	G.SceneNormal.InitAsRenderTarget(G.ScreenWidth, G.ScreenHeight, RenderFormat::R16G16B16A16_FLOAT, L"SceneNormal");
	G.SceneRoughnessMetallic.InitAsRenderTarget(G.ScreenWidth, G.ScreenHeight, RenderFormat::R16G16_FLOAT, L"SceneRoughnessMetallic");
	G.SceneShadow0.InitAsCompute(G.ScreenWidth, G.ScreenHeight, RenderFormat::R8_UNORM, L"SceneShadow0");
	G.SceneShadow1.InitAsCompute(G.ScreenWidth, G.ScreenHeight, RenderFormat::R8_UNORM, L"SceneShadow1");
	G.SceneVelocity.InitAsRenderTarget(G.ScreenWidth, G.ScreenHeight, RenderFormat::R16G16_FLOAT, L"SceneVelocity");
	G.SceneDepth0.InitAsDepth(G.ScreenWidth, G.ScreenHeight, RenderFormat::D32_FLOAT, RenderFormat::R32_FLOAT, L"SceneDepth0");
	G.SceneDepth1.InitAsDepth(G.ScreenWidth, G.ScreenHeight, RenderFormat::D32_FLOAT, RenderFormat::R32_FLOAT, L"SceneDepth1");
	G.SceneConfidence.InitAsCompute(G.ScreenWidth, G.ScreenHeight, RenderFormat::R8_UNORM, L"SceneConfidence");

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

void Render(rl::RenderView* view, rl::CommandListSubmissionGroup* clGroup, float deltaSeconds)
{
	float3 SunDirection = GetSunDirection();
	matrix ViewProjection = G.Cam.GetView() * G.Cam.GetProjection();

	const uint64_t FrameID = view->GetFrameID();
	SceneTarget_s& CurrentFrameDepth = FrameID % 2 == 0 ? G.SceneDepth0 : G.SceneDepth1;
	SceneTarget_s& PrevFrameDepth = FrameID % 2 == 0 ? G.SceneDepth1 : G.SceneDepth0;
	SceneTarget_s& CurrentFrameShadow = FrameID % 2 == 0 ? G.SceneShadow0 : G.SceneShadow1;
	SceneTarget_s& PrevFrameShadow = FrameID % 2 == 0 ? G.SceneShadow1 : G.SceneShadow0;

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

	CommandList* MainCL = clGroup->CreateCommandList();

	MainCL->SetRootSignature();

	// Set up resources
	{
		CurrentFrameDepth.Transition(MainCL, ResourceTransitionState::DEPTH_WRITE);
	}

	// Bind and clear targets
	{
		constexpr float DefaultClearCol[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		MainCL->ClearRenderTarget(G.SceneColor.RTV, DefaultClearCol);
		MainCL->ClearRenderTarget(G.SceneNormal.RTV, DefaultClearCol);
		MainCL->ClearRenderTarget(G.SceneRoughnessMetallic.RTV, DefaultClearCol);
		MainCL->ClearRenderTarget(G.SceneVelocity.RTV, DefaultClearCol);
		MainCL->ClearDepth(CurrentFrameDepth.DSV, 1.0f);

		RenderTargetView_t SceneTargets[] = { G.SceneColor.RTV, G.SceneNormal.RTV, G.SceneRoughnessMetallic.RTV, G.SceneVelocity.RTV };
		MainCL->SetRenderTargets(SceneTargets, ARRAYSIZE(SceneTargets), CurrentFrameDepth.DSV);
	}

	// Init viewport and scissor
	{
		Viewport vp{ G.ScreenWidth, G.ScreenHeight };
		MainCL->SetViewports(&vp, 1);
		MainCL->SetDefaultScissor();
	}

	// Bind draw buffers
	{
		MainCL->SetGraphicsRootCBV(GlobalRootSigSlots::RS_VIEW_BUF, ViewCBuf);
		MainCL->SetGraphicsRootDescriptorTable(GlobalRootSigSlots::RS_SRV_TABLE);
		MainCL->SetComputeRootDescriptorTable(GlobalRootSigSlots::RS_UAV_TABLE);
	}

	if (G.UseMeshShaders)
	{
		MainCL->SetPipelineState(G.MeshMSPSO);
	}
	else
	{
		MainCL->SetPipelineState(G.MeshVSPSO);
	}

	for (const RTDModel_s& Model : G.Models)
	{
		Model.Draw(MainCL);
	}

	// Transition for deferred pass
	{
		G.SceneColor.Transition(MainCL, ResourceTransitionState::PIXEL_SHADER_RESOURCE);
		G.SceneNormal.Transition(MainCL, ResourceTransitionState::PIXEL_SHADER_RESOURCE);
		G.SceneRoughnessMetallic.Transition(MainCL, ResourceTransitionState::PIXEL_SHADER_RESOURCE);
		G.SceneVelocity.Transition(MainCL, ResourceTransitionState::NON_PIXEL_SHADER_RESOURCE);
		CurrentFrameDepth.Transition(MainCL, ResourceTransitionState::NON_PIXEL_SHADER_RESOURCE);


		CurrentFrameShadow.Transition(MainCL, ResourceTransitionState::UNORDERED_ACCESS);
	}

	// RT Shadows
	if (!G.ShowShadows)
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
		ClearUniforms.UAVIndex = GetDescriptorIndex(CurrentFrameShadow.UAV);
		ClearUniforms.Width = G.ScreenWidth;
		ClearUniforms.Height = G.ScreenHeight;
		ClearUniforms.ClearVal = 1.0f;
		DynamicBuffer_t ClearBuf = CreateDynamicConstantBuffer(&ClearUniforms);

		MainCL->SetPipelineState(G.UAVClearF1PSO);
		MainCL->SetComputeRootCBV(GlobalRootSigSlots::RS_VIEW_BUF, ClearBuf);
		MainCL->Dispatch(DivideRoundUp(G.ScreenWidth, 8u), DivideRoundUp(G.ScreenHeight, 8u), 1u);
	}
	else
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
		RayUniforms.SceneDepthTextureIndex = GetDescriptorIndex(CurrentFrameDepth.SRV);
		RayUniforms.SceneShadowTextureIndex = GetDescriptorIndex(CurrentFrameShadow.UAV);
		RayUniforms.Time = G.ElapsedTime;
		RayUniforms.AccumFrames = (float)G.FramesSinceMove;

		DynamicBuffer_t RayCBuf = CreateDynamicConstantBuffer(&RayUniforms);

		CommandList* RTCL = clGroup->CreateCommandList();

		RTCL->SetComputeRootSignature(G.RTRootSignature);
		RTCL->SetComputeRootDescriptorTable(RTRootSigSlots::RS_UAV_TABLE);

		RTCL->SetPipelineState(G.RTPSO);

		RTCL->SetComputeRootCBV(RTRootSigSlots::RS_CONSTANTS, RayCBuf);
		RTCL->SetComputeRootSRV(RTRootSigSlots::RS_RAYTRACING_SCENE, G.RaytracingScene);
		RTCL->SetComputeRootDescriptorTable(RTRootSigSlots::RS_SRV_TABLE);

		RTCL->DispatchRays(G.RaytracingShaderTable, G.ScreenWidth, G.ScreenHeight, 1);
	}

	CommandList* DeferredCL = clGroup->CreateCommandList();

	DeferredCL->SetRootSignature();
	DeferredCL->SetGraphicsRootDescriptorTable(GlobalRootSigSlots::RS_SRV_TABLE);

	DeferredCL->SetComputeRootDescriptorTable(GlobalRootSigSlots::RS_SRV_TABLE);
	DeferredCL->SetComputeRootDescriptorTable(GlobalRootSigSlots::RS_UAV_TABLE);

	PrevFrameDepth.Transition(DeferredCL, ResourceTransitionState::NON_PIXEL_SHADER_RESOURCE);
	PrevFrameShadow.Transition(DeferredCL, ResourceTransitionState::NON_PIXEL_SHADER_RESOURCE);

	// Denoise
	{
		DeferredCL->UAVBarrier(CurrentFrameShadow.Texture);
		G.SceneConfidence.Transition(DeferredCL, ResourceTransitionState::UNORDERED_ACCESS);

		struct DenoiseUniforms_s
		{
			matrix CamToWorld;
			matrix PrevCamToWorld;

			uint32_t DepthTextureIndex;
			uint32_t PrevFrameDepthTextureIndex;
			uint32_t ShadowTextureIndex;
			uint32_t PrevFrameShadowTextureIndex;

			uint32_t ConfidenceTextureIndex;
			uint32_t VelocityTextureIndex;
			float2 ViewportSizeRcp;

			uint32_t ViewportWidth;
			uint32_t ViewportHeight;
			float __Pad0[2];
		} DenoiseUniforms;

		DenoiseUniforms.CamToWorld = InverseMatrix(ViewProjection);
		DenoiseUniforms.PrevCamToWorld = InverseMatrix(G.PrevViewProjection);
		DenoiseUniforms.DepthTextureIndex = GetDescriptorIndex(CurrentFrameDepth.SRV);
		DenoiseUniforms.PrevFrameDepthTextureIndex = GetDescriptorIndex(PrevFrameDepth.SRV);
		DenoiseUniforms.ShadowTextureIndex = GetDescriptorIndex(CurrentFrameShadow.UAV);
		DenoiseUniforms.PrevFrameShadowTextureIndex = GetDescriptorIndex(PrevFrameShadow.SRV);
		DenoiseUniforms.ConfidenceTextureIndex = GetDescriptorIndex(G.SceneConfidence.UAV);
		DenoiseUniforms.VelocityTextureIndex = GetDescriptorIndex(G.SceneVelocity.SRV);
		DenoiseUniforms.ViewportSizeRcp = float2(1.0f / (float)G.ScreenWidth, 1.0f / (float)G.ScreenHeight);
		DenoiseUniforms.ViewportWidth = G.ScreenWidth;
		DenoiseUniforms.ViewportHeight = G.ScreenHeight;

		DynamicBuffer_t DenoiseCBuf = CreateDynamicConstantBuffer(&DenoiseUniforms);

		DeferredCL->SetComputeRootCBV(GlobalRootSigSlots::RS_VIEW_BUF, DenoiseCBuf);
		DeferredCL->SetPipelineState(G.ShadowDenoisePSO);

		DeferredCL->Dispatch(DivideRoundUp(G.ScreenWidth, 8u), DivideRoundUp(G.ScreenHeight, 8u), 1u);
	}

	Viewport vp{ G.ScreenWidth, G.ScreenHeight };
	DeferredCL->SetViewports(&vp, 1);
	DeferredCL->SetDefaultScissor();	

	// Bind back buffer target
	{
		constexpr float DefaultClearCol[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

		RenderTargetView_t backBufferRtv = view->GetCurrentBackBufferRTV();

		DeferredCL->ClearRenderTarget(backBufferRtv, DefaultClearCol);

		DeferredCL->SetRenderTargets(&backBufferRtv, 1, DepthStencilView_t::INVALID);
	}

	CurrentFrameShadow.Transition(DeferredCL, ResourceTransitionState::PIXEL_SHADER_RESOURCE);
	G.SceneConfidence.Transition(DeferredCL, ResourceTransitionState::PIXEL_SHADER_RESOURCE);

	// Render deferred
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
		DeferredConsts.SceneColorTextureIndex = GetDescriptorIndex(G.SceneColor.SRV);
		DeferredConsts.SceneNormalTextureIndex = GetDescriptorIndex(G.SceneNormal.SRV);
		DeferredConsts.SceneRoughnessMetallicTextureIndex = GetDescriptorIndex(G.SceneRoughnessMetallic.SRV);
		DeferredConsts.DepthTextureIndex = GetDescriptorIndex(CurrentFrameDepth.SRV);
		DeferredConsts.DrawMode = G.DrawMode;
		DeferredConsts.CamPosition = G.Cam.GetPosition();
		DeferredConsts.ShadowTexture = GetDescriptorIndex(CurrentFrameShadow.SRV);
		DeferredConsts.SunDirection = SunDirection;
		DeferredConsts.VelocityTextureIndex = GetDescriptorIndex(G.SceneVelocity.SRV);
		DeferredConsts.ConfidenceTextureIndex = GetDescriptorIndex(G.SceneConfidence.SRV);
		DeferredConsts.ViewportSizeRcp = float2(1.0f / (float)G.ScreenWidth, 1.0f / (float)G.ScreenHeight);

		DynamicBuffer_t DeferredCBuf = CreateDynamicConstantBuffer(&DeferredConsts);

		DeferredCL->SetGraphicsRootCBV(GlobalRootSigSlots::RS_VIEW_BUF, DeferredCBuf);

		DeferredCL->SetGraphicsRootDescriptorTable(GlobalRootSigSlots::RS_SRV_TABLE);

		DeferredCL->SetPipelineState(G.DeferredPSO);

		DeferredCL->DrawInstanced(6u, 1u, 0u, 0u);
	}

	// Transition for next pass
	{
		G.SceneColor.Transition(DeferredCL, ResourceTransitionState::RENDER_TARGET);
		G.SceneNormal.Transition(DeferredCL, ResourceTransitionState::RENDER_TARGET);
		G.SceneRoughnessMetallic.Transition(DeferredCL, ResourceTransitionState::RENDER_TARGET);
		G.SceneVelocity.Transition(DeferredCL, ResourceTransitionState::RENDER_TARGET);
	}

	G.PrevViewProjection = ViewProjection;
}

void ShutdownApp()
{

}
