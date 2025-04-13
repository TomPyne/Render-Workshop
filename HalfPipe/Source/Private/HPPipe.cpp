#include "HPPipe.h"

#include "HPModelPipe.h"

#include <Logging/Logging.h>

#include <map>

struct HPPipeGlobals_s
{
	std::map<std::wstring, IHPPipe_c*> Pipes;
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
