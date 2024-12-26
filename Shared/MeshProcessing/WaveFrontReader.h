#pragma once

#include <fstream>
#include <string>
#include <SurfMath.h>
#include <unordered_map>
#include <vector>

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

		bool HasSpecular;
		bool HasEmissive;

		std::wstring Name;
		std::wstring Texture;
		std::wstring NormalTexture;
		std::wstring SpecularTexture;
		std::wstring EmissiveTexture;
		std::wstring RMATexture;

		Material_s() noexcept :
			Ambient(0.2f, 0.2f, 0.2f),
			Diffuse(0.8f, 0.8f, 0.8f),
			Specular(1.0f, 1.0f, 1.0f),
			Emissive(0.f, 0.f, 0.f),
			Shininess(0),
			Alpha(1.f),
			HasSpecular(false),
			HasEmissive(false),
			Name{},
			Texture{},
			NormalTexture{},
			SpecularTexture{},
			EmissiveTexture{},
			RMATexture{}
		{
		}
	};

	std::vector<Vertex_s>   Vertices;
	std::vector<index_t>	Indices;
	std::vector<uint32_t>	Attributes;
	std::vector<Material_s> Materials;

	std::wstring            Name;
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
	void LoadTexturePath(std::wifstream& InFile, std::wstring& Texture);
};
