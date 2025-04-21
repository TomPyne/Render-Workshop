#pragma once

#include <fstream>
#include <string>
#include <SurfMath.h>
#include <unordered_map>
#include <vector>

class WaveFrontMtlReader_c
{
public:

	struct Material_s
	{
		float3 Ambient = float3(0.2f);
		float3 Diffuse = float3(0.8f);
		float3 Specular = float3(1.0f);
		float3 Emissive = float3(0.0f);
		u32 Shininess = 0;
		float Alpha = 1.0f;
		float BumpScale = 1.0f;
		float Roughness = 1.0f;
		float Metallic = 1.0f;

		bool HasSpecular = false;
		bool HasEmissive = false;

		std::wstring Name;
		std::wstring DiffuseTexture;
		std::wstring NormalTexture;
		std::wstring SpecularTexture;
		std::wstring EmissiveTexture;
		std::wstring RMATexture;
		std::wstring RoughnessTexture;
		std::wstring MetallicTexture;
		std::wstring BumpTexture;
	};

	std::vector<Material_s> Materials;

	bool Load(const wchar_t* FileName);

private:
	void LoadTexturePath(std::wifstream& InFile, const std::wstring& BasePath, std::wstring& Texture);
	void LoadBumpTexturePath(std::wifstream& InFile, const std::wstring& BasePath, float& Scale, std::wstring& Texture);
};

class WaveFrontReader_c
{
public:

	using index_t = uint32_t;

	struct Vertex_s
	{
		float3 Position;
		float3 Normal;
		float2 Texcoord;
	};

	struct Material_s
	{
		float3 Ambient;
		float3 Diffuse;
		float3 Specular;
		float3 Emissive;
		u32 Shininess;
		float Alpha;
		float BumpScale;
		float Roughness;
		float Metallic;

		bool HasSpecular;
		bool HasEmissive;

		std::wstring Name;
		std::wstring DiffuseTexture;
		std::wstring NormalTexture;
		std::wstring SpecularTexture;
		std::wstring EmissiveTexture;
		std::wstring RMATexture;
		std::wstring RoughnessTexture;
		std::wstring MetallicTexture;
		std::wstring BumpTexture;

		Material_s() noexcept :
			Ambient(0.2f, 0.2f, 0.2f),
			Diffuse(0.8f, 0.8f, 0.8f),
			Specular(1.0f, 1.0f, 1.0f),
			Emissive(0.f, 0.f, 0.f),
			Shininess(0),
			Alpha(1.f),
			BumpScale(1.f),
			Roughness(1.f),
			Metallic(1.f),
			HasSpecular(false),
			HasEmissive(false),
			Name{},
			DiffuseTexture{},
			NormalTexture{},
			SpecularTexture{},
			EmissiveTexture{},
			RMATexture{},
			BumpTexture{}
		{
		}
	};

	std::vector<Vertex_s>   Vertices;
	std::vector<index_t>	Indices;
	std::vector<uint32_t>	Attributes;
	std::vector<Material_s> Materials;

	std::wstring            Name;
	std::wstring			MtlFile;
	bool                    HasNormals;
	bool                    HasTexcoords;

	BoundingBox				Bounds;

	WaveFrontReader_c() : HasNormals(false), HasTexcoords(false) {}

	bool Load(const wchar_t* FileName);

	bool LoadMTL(const wchar_t* FileName);

	void Clear();

private:

	using VertexCache_t = std::unordered_multimap<uint32_t, uint32_t>;

	uint32_t AddVertex(uint32_t Hash, const Vertex_s* Vertex, VertexCache_t& Cache);
	void LoadTexturePath(std::wifstream& InFile, const std::wstring& BasePath, std::wstring& Texture);
	void LoadBumpTexturePath(std::wifstream& InFile, const std::wstring& BasePath, float& Scale, std::wstring& Texture);
};
