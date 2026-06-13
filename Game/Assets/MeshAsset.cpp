#include "MeshAsset.h"

#include <HalfPipe/Source/Public/WaveFrontReader.h>
#include <Shared/Logging/Logging.h>
#include <Shared/StringUtils/StringUtils.h>

#include <json.hpp>

#include <fstream>

MeshAsset_s* MeshAsset::RequestMeshAsset(const std::wstring& Path)
{
	using json = nlohmann::json;
    std::wifstream AssetFile(Path.c_str());

	//json Data = json::parse(AssetFile);

	//std::string SourcePath = "";

	// Meh use halfpipe assets only

	//bool Success = false;
	//do
	//{
	//	if (Data.contains("SourceType"))
	//	{
	//		if (stricmp(Data["SourceType"].get<std::string>().c_str(), "Obj") != 0)
	//		{
	//			LOGERROR("[MeshAsset] Unsupported source mesh type");
	//			break;
	//		}
	//	}
	//	else
	//	{
	//		LOGERROR("[MeshAsset] Requires SourceType field");
	//		break;
	//	}

	//	if (Data.contains("SourcePath"))
	//	{
	//		SourcePath = Data["SourcePath"].get<std::string>();
	//	}
	//	else
	//	{
	//		LOGERROR("[MeshAsset] Requires SourcePath field");
	//		break;
	//	}

	//	if (SourcePath.empty())
	//	{
	//		LOGERROR("[MeshAsset] SourcePath field is empty");
	//		break;
	//	}

	//	WaveFrontReader_c ObjReader = WaveFrontReader_c();
	//	std::wstring SourcePathW = NarrowToWide(SourcePath);
	//	if (!ObjReader.Load(SourcePathW.c_str()))
	//	{
	//		LOGERROR("[MeshAsset] Failed to load obj");
	//		break;
	//	}

	//	Success = true;
	//} while (false);

	return nullptr;
}
