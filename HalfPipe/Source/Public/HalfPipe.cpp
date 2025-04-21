#include "HalfPipe.h"
#include "HPPipe.h"

void HPCook(const std::wstring& SourceDir, const std::wstring& OutputDir, const std::vector<HPAssetArgs_s>& Args)
{
	InitPipes();	

	for (const HPAssetArgs_s& AssetArgs : Args)
	{
		PushCookCommand(AssetArgs);
	}

	ProcessCookCommands(SourceDir, OutputDir);
}
