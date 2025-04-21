#include "HPWfMtlLibPipe.h"

#include "HPWfMtlLib.h"

#include "WaveFrontReader.h"

#include <FileUtils/FileStream.h>
#include <FileUtils/PathUtils.h>
#include <Logging/Logging.h>

static std::wstring GenerateOutputPath(const std::wstring& OutputDir, const std::wstring& AssetPath)
{
	return OutputDir + L"/" + ReplacePathExtension(AssetPath, L"hp_wml");
}

void HPWfMtlLibPipe_c::Cook(const std::wstring& SourceDir, const std::wstring& OutputDir, const HPArgs_t& Args)
{
	std::wstring AssetPath;
	if (!ParseArgs(Args, L"-src", AssetPath))
	{
		LOGERROR("[HPWfMtlLibPipe] No source path provided for asset");
		return;
	}

	if (AssetPath.empty())
	{
		LOGERROR("[HPWfMtlLibPipe] No source path provided for asset");
	}

	std::wstring AbsSrcPath = SourceDir + L"/" + AssetPath;

	HPWfMtlLib_s CookedAsset = {};

	if (!HasPathExtension(AbsSrcPath, L".mtl"))
	{
		LOGERROR("[HPWfMtlLibPipe] Unsupported file extension for source asset [%S]", AbsSrcPath.c_str());
		return;
	}

	WaveFrontMtlReader_c MtlReader;
	if (!MtlReader.Load(AbsSrcPath.c_str()))
	{
		LOGERROR("[HPWfMtlLibPipe] Failed to parse mtl file [%S]", AbsSrcPath.c_str());
		return;
	}

	CookedAsset.Materials.resize(MtlReader.Materials.size());

	for (size_t MaterialIt = 0; MaterialIt < MtlReader.Materials.size(); MaterialIt++)
	{
		auto RequestTexture = [&](const std::wstring& TexturePath) -> std::wstring
		{
			if (TexturePath.empty())
			{
				return {};
			}
			std::wstring AbsolutePath = MakePathAbsolute(TexturePath);
			std::wstring ContentRelativePath = MakePathRelativeTo(AbsolutePath, SourceDir);
			HPAssetArgs_s MtlLibArgs;
			MtlLibArgs.AssetType = L"Texture";
			MtlLibArgs.Args.push_back(L"-src");
			MtlLibArgs.Args.push_back(ContentRelativePath);
			PushCookCommand(MtlLibArgs);

			return GetPackagePathForAssetFromArgs(MtlLibArgs);
		};

		CookedAsset.Materials[MaterialIt].Name = MtlReader.Materials[MaterialIt].Name;
		CookedAsset.Materials[MaterialIt].DiffuseTexture = RequestTexture(MtlReader.Materials[MaterialIt].DiffuseTexture);
		CookedAsset.Materials[MaterialIt].SpecularTexture = RequestTexture(MtlReader.Materials[MaterialIt].SpecularTexture);
		CookedAsset.Materials[MaterialIt].NormalTexture = RequestTexture(MtlReader.Materials[MaterialIt].BumpTexture);
	}

	std::wstring AssetOutputPath = GenerateOutputPath(OutputDir, AssetPath);

	CreateDirectories(AssetOutputPath);

	if (!CookedAsset.Serialize(AssetOutputPath, FileStreamMode_e::WRITE))
	{
		LOGERROR("[HPWfMtlLibPipe] Failed to write asset [%S]", AssetOutputPath.c_str());
		return;
	}

	LOGINFO("[HPWfMtlLibPipe] Cooked material [%S]", AssetOutputPath.c_str());
}

std::wstring HPWfMtlLibPipe_c::GetCookedAssetPath(const std::wstring& OutputDir, const HPArgs_t& Args) const
{
	std::wstring AssetPath;
	if (!ParseArgs(Args, L"-src", AssetPath))
	{
		LOGERROR("[HPWfMtlLibPipe] No source path provided for asset");
		return {};
	}

	return GenerateOutputPath(OutputDir, AssetPath);
}

std::wstring HPWfMtlLibPipe_c::GetPackageAssetPath(const HPArgs_t& Args) const
{
	std::wstring AssetPath;
	if (!ParseArgs(Args, L"-src", AssetPath))
	{
		LOGERROR("[HPWfMtlLibPipe] No source path provided for asset");
		return {};
	}
	return ReplacePathExtension(AssetPath, L"hp_wml");
}
