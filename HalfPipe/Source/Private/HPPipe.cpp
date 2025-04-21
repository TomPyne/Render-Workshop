#include "HPPipe.h"

#include "HPModelPipe.h"
#include "HPTexturePipe.h"
#include "HPWfMtlLibPipe.h"

#include <Logging/Logging.h>

#include <map>
#include <queue>

struct HPPipeGlobals_s
{
	std::map<std::wstring, IHPPipe_c*> Pipes;

	std::queue<HPAssetArgs_s> CookCommands;
	uint32_t CookCount = 0;
} G;

template<class Pipe_t>
void RegisterPipe()
{
	Pipe_t* Pipe = new Pipe_t;
	if (G.Pipes.contains(Pipe->GetAssetType()))
	{
		delete Pipe;
		return;
	}

	G.Pipes[Pipe->GetAssetType()] = Pipe;
}

void InitPipes()
{
	RegisterPipe<HPModelPipe_c>();
	RegisterPipe<HPWfMtlLibPipe_c>();
	RegisterPipe<HPTexturePipe_c>();
}

IHPPipe_c* GetPipeForAsset(const wchar_t* AssetType)
{
	if (!AssetType)
	{
		LOGERROR("GetPipeForAsset AssetType null", AssetType);
		return nullptr;
	}		

	auto It = G.Pipes.find(AssetType);
	if (It != G.Pipes.end())
		return It->second;

	LOGERROR("GetPipeForAsset failed to find pipe for asset type [%S]", AssetType);
	return nullptr;
}

std::wstring GetCookedPathForAssetFromArgs(const std::wstring& OutputDir, const HPAssetArgs_s& AssetArgs)
{
	if (IHPPipe_c* Pipe = GetPipeForAsset(AssetArgs.AssetType.c_str()))
	{
		return Pipe->GetCookedAssetPath(OutputDir, AssetArgs.Args);
	}

	return {};
}

std::wstring GetPackagePathForAssetFromArgs(const HPAssetArgs_s& AssetArgs)
{
	if (IHPPipe_c* Pipe = GetPipeForAsset(AssetArgs.AssetType.c_str()))
	{
		return Pipe->GetPackageAssetPath(AssetArgs.Args);
	}

	return {};
}

void PushCookCommand(const HPAssetArgs_s& Args)
{
	G.CookCommands.push(Args);
}

void ProcessCookCommands(const std::wstring& SourceDir, const std::wstring& OutputDir)
{
	G.CookCount = 0;
	while (!G.CookCommands.empty())
	{
		LOGINFO("ProcessCookCommands - Cooked %d - Remain %d", G.CookCount, static_cast<uint32_t>(G.CookCommands.size()));

		if (IHPPipe_c* Pipe = GetPipeForAsset(G.CookCommands.front().AssetType.c_str()))
		{
			Pipe->Cook(SourceDir, OutputDir, G.CookCommands.front().Args);

			G.CookCount++;
		}
		else
		{
			break;
		}
		G.CookCommands.pop();
	}
}

bool IHPPipe_c::ParseArgs(const HPArgs_t& Args, const wchar_t* Command, std::wstring& OutValue)
{
	for (size_t ArgIt = 0; ArgIt < Args.size(); ArgIt++)
	{
		if (wcscmp(Args[ArgIt].c_str(), Command) == 0)
		{
			ArgIt++;
			if (ArgIt >= Args.size())
			{
				return false;
			}

			OutValue = Args[ArgIt];
			return true;
		}
	}

	return false;
}
