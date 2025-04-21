#pragma once

#include <string>
#include <vector>

enum class HPWfMtlLibVersion_e
{
	INITIAL = 0,

	CURRENT = INITIAL,
};

enum class FileStreamMode_e;

struct HPWfMtlLib_s
{
	HPWfMtlLibVersion_e Version;

	struct Material_s
	{
		std::wstring DiffuseTexture;
		std::wstring SpecularTexture;
		std::wstring NormalTexture;
	};

	std::vector<Material_s> Materials;


	std::wstring SourcePath;

	bool Serialize(const std::wstring& Path, FileStreamMode_e Mode);
};