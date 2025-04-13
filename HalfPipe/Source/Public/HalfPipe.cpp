#include "HalfPipe.h"
#include "HPPipe.h"

void HPCook(const std::wstring& SourceDir, const std::wstring& OutputDir, const std::vector<HPAssetArgs_s>& Args)
{
	InitPipes();

	for (const HPAssetArgs_s Asset : Args)
	{
		if (IHPPipe_c* Pipe = GetPipeForAsset(Asset.AssetType.c_str()))
		{
			Pipe->Cook(SourceDir, OutputDir, Asset.Args);
		}
	}
}
