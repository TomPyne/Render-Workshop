#include "HPTexturePipe.h"

#include "HPTexture.h"

#include <FileUtils/FileStream.h>
#include <FileUtils/PathUtils.h>
#include <Logging/Logging.h>
#include <TextureUtils/DDSTextureLoader.h>

static std::wstring GenerateOutputPath(const std::wstring& OutputDir, const std::wstring& AssetPath)
{
	return OutputDir + L"/" + ReplacePathExtension(AssetPath, L"hp_tex");
}

void HPTexturePipe_c::Cook(const std::wstring& SourceDir, const std::wstring& OutputDir, const HPArgs_t& Args)
{
	std::wstring AssetPath;
	if (!ParseArgs(Args, L"-src", AssetPath))
	{
		LOGERROR("[HPTexturePipe] No source path provided for asset");
		return;
	}

	std::wstring AbsSrcPath = SourceDir + L"/" + AssetPath;

	HPTexture_s CookedAsset = {};

	const wchar_t* SupportedExtensions[] =
	{
		L".dds",
	};

	bool SupportedExt = false;
	for (size_t ExtIt = 0; ExtIt < ARRAYSIZE(SupportedExtensions); ExtIt++)
	{
		if (HasPathExtension(AssetPath, SupportedExtensions[ExtIt]))
		{
			SupportedExt = true;
		}
	}

	if (!SupportedExt)
	{
		LOGERROR("[HPTexturePipe] Unsupported path extension for texture %S", AssetPath.c_str());
		return;
	}

	DDSTexture_s LoadedTexture;
	if (!LoadDDSTexture(AbsSrcPath.c_str(), &LoadedTexture))
	{
		LOGERROR("[HPTexturePipe] Failed to load DDS texture %S", AssetPath.c_str());
		return;
	}

	CookedAsset.SourcePath = AssetPath;
	CookedAsset.Width = LoadedTexture.Width;
	CookedAsset.Height = LoadedTexture.Height;
	CookedAsset.Format = LoadedTexture.Format;

	for (const DDSTexture_s::Subresource_s& SubRes : LoadedTexture.SubResourceInfos)
	{
		CookedAsset.Mips.emplace_back(SubRes.DataOffset, SubRes.RowPitch, SubRes.SlicePitch);
	}

	std::swap(CookedAsset.Data, LoadedTexture.Data);

	std::wstring AssetOutputPath = GenerateOutputPath(OutputDir, AssetPath);

	CreateDirectories(AssetOutputPath);

	if (!CookedAsset.Serialize(AssetOutputPath, FileStreamMode_e::WRITE))
	{
		LOGERROR("[HPTexturePipe] Failed to write asset [%S]", AssetOutputPath.c_str());
		return;
	}

	LOGINFO("[HPTexturePipe] Cooked texture [%S]", AssetOutputPath.c_str());
}

std::wstring HPTexturePipe_c::GetCookedAssetPath(const std::wstring& OutputDir, const HPArgs_t& Args) const
{
	std::wstring AssetPath;
	if (!ParseArgs(Args, L"-src", AssetPath))
	{
		LOGERROR("[HPTexturePipe] No source path provided for asset");
		return {};
	}

	return GenerateOutputPath(OutputDir, AssetPath);
}

std::wstring HPTexturePipe_c::GetPackageAssetPath(const HPArgs_t& Args) const
{
	std::wstring AssetPath;
	if (!ParseArgs(Args, L"-src", AssetPath))
	{
		LOGERROR("[HPTexturePipe] No source path provided for asset");
		return {};
	}
	return ReplacePathExtension(AssetPath, L"hp_tex");
}
