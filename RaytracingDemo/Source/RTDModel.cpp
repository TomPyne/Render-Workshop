#include "RTDModel.h"
#include "RTDGlobals.h"

#include <FileUtils/FileStream.h>
#include <HPModel.h>
#include <HPTexture.h>
#include <Logging/Logging.h>
#include <Render/Raytracing.h>
#include <Render/Render.h>

bool RTDTexture_s::Init(const HPTexture_s& Asset)
{
	rl::TextureCreateDescEx TexDesc = {};
	TexDesc.DebugName = Asset.SourcePath;
	TexDesc.Width = Asset.Width;
	TexDesc.Height = Asset.Height;
	TexDesc.DepthOrArraySize = 1u;
	TexDesc.MipCount = static_cast<uint32_t>(Asset.Mips.size());
	TexDesc.Dimension = rl::TextureDimension::TEX2D;
	TexDesc.Flags = rl::RenderResourceFlags::SRV;
	TexDesc.ResourceFormat = Asset.Format;

	std::vector<rl::MipData> Data;
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

			auto TexIt = Glob.TextureMap.find(TexPath);
			if (TexIt != Glob.TextureMap.end())
			{
				return TexIt->second;
			}

			std::shared_ptr<RTDTexture_s> NewTexture = std::make_shared<RTDTexture_s>();
			if (NewTexture->Init(TextureAsset))
			{
				Glob.TextureMap[TexPath] = NewTexture;
			}
			else
			{
				Glob.TextureMap[TexPath] = nullptr;
			}

			return Glob.TextureMap[TexPath];
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
	RTDesc.VertexFormat = rl::RenderFormat::R32G32B32_FLOAT;
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

			rl::AddRaytracingGeometryToScene(Mesh.RaytracingGeometry, Glob.RaytracingScene);
		}

		std::wstring MaterialKey = Asset->MaterialLibPath + MeshFromAsset.LibMaterialName;
		auto It = Glob.MaterialMap.find(MaterialKey);
		if (It != Glob.MaterialMap.end())
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

			Glob.MaterialMap[MaterialKey] = LoadedMaterial;
			Mesh.Material = LoadedMaterial;
		}

		Meshes.push_back(Mesh);
	}

	return true;
}

void RTDBuffer_s::InitInternal(const void* const Data, size_t Count, size_t ElemSize)
{
	CHECK(Data != nullptr);
	StructuredBuffer = rl::CreateStructuredBuffer(Data, ElemSize * Count, ElemSize, rl::RenderResourceFlags::SRV);
	CHECK(StructuredBuffer);
	BufferSRV = rl::CreateStructuredBufferSRV(StructuredBuffer, 0u, static_cast<uint32_t>(Count), static_cast<uint32_t>(ElemSize));
	CHECK(BufferSRV);
}
