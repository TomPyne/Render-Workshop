#pragma once

#include <Render/Render.h>
#include <string>

enum class HPTextureVersion_e
{
	INITIAL = 0,

	CURRENT = INITIAL,
};

enum class FileStreamMode_e;

struct HPTexture_s
{
	HPTextureVersion_e Version = HPTextureVersion_e::CURRENT;

	uint32_t Width;
	uint32_t Height;
	tpr::RenderFormat Format;

	struct MipData_s
	{
		uint32_t Offset;
		uint32_t RowPitch;
		uint32_t SlicePitch;
	};

	std::vector<MipData_s> Mips;
	std::vector<uint8_t> Data;

	std::wstring SourcePath;

	bool Serialize(const std::wstring& Path, FileStreamMode_e Mode);
};