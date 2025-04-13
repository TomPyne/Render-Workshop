#pragma once

#include "HalfPipe.h"

#include <Logging/Logging.h>
#include <fstream>
#include <string>
#include <vector>

struct CookFile_s
{
	std::string FullPath;
	std::string RelativePath;
};

int wmain(int argc, wchar_t* argv[])
{
	LOGINFO("HalfPipeApp Startup");

	LoggingEnableConsole(true);
	LoggingEnablePrettyPrint(true);

	std::vector<std::wstring> PackagesToCook;
	std::vector<HPAssetArgs_s> AssetsToCook;

	std::wstring SrcDir;
	std::wstring OutDir;

	if (argc < 5)
	{
		LOGERROR("Not enough commands specified");
		return 1;
	}

	{
		std::wstring SrcCommand = argv[1];
		if (SrcCommand != L"-s")
		{
			LOGERROR("First command should be -s source/path");
			return 1;
		}
		SrcDir = argv[2];

		std::wstring OutCommand = argv[3];
		if (OutCommand != L"-o")
		{
			LOGERROR("Second command should be -o source/path");
			return 1;
		}
		OutDir = argv[4];

		LOGINFO("Source dir: %S", SrcDir.c_str());
		LOGINFO("Out dir: %S", OutDir.c_str());
	}

	LOGINFO("Commands:");
	for (int arg = 5; arg < argc;)
	{
		if (arg + 1 >= argc)
			break;		

		std::wstring Command = argv[arg];
		std::wstring Input = argv[arg + 1];

		if (Command == L"+p")
		{
			PackagesToCook.push_back(Input);
			LOGINFO("Package: %S", Input.c_str());

			arg += 2;
		}		
		else if (Command == L"+a")
		{
			HPAssetArgs_s Asset;
			Asset.AssetType = Input;
			arg += 2;

			while (arg < argc)
			{
				std::wstring ArgInput = argv[arg];

				if (ArgInput == L"+a" || ArgInput == L"+p")
				{
					break;
				}

				Asset.Args.push_back(ArgInput);

				arg++;
			}

			AssetsToCook.push_back(Asset);
			LOGINFO("Asset: %S", Input.c_str());
		}
	}

	for(const std::wstring& Package : PackagesToCook)
	{
		std::vector<std::wstring> FilesToCook;

		std::wifstream ListStream = std::wifstream(SrcDir + L"/" + Package);
		if (ListStream.is_open())
		{
			HPAssetArgs_s* CurrentAsset = nullptr;
			int32_t CurrentLine = 0;
			for (std::wstring Line; std::getline(ListStream, Line); CurrentLine++)
			{
				if (Line.empty())
					continue;

				if (!Line.starts_with(L"-"))
				{
					AssetsToCook.push_back({});
					CurrentAsset = &AssetsToCook.back();
					CurrentAsset->AssetType = Line;
				}
				else
				{
					if (!CurrentAsset)
					{
						LOGERROR("Error parsing Package [%S] line: %d, incorrect file layout", Package.c_str(), CurrentLine);
						break; // TODO: handle failure, return 0?
					}
					size_t ArgIndex = Line.find_first_of(L' ');
					if (ArgIndex == std::wstring::npos || ArgIndex >= (Line.size() - 1))
					{
						LOGERROR("Error parsing Package [%S] line: %d, missing arg for command", Package.c_str(), CurrentLine);
						break;
					}

					std::wstring Command = Line.substr(0, ArgIndex);
					std::wstring Arg = Line.substr(ArgIndex + 1);
					CurrentAsset->Args.push_back(Command);
					CurrentAsset->Args.push_back(Arg);
				}
			}
		}
	}

	HPCook(SrcDir, OutDir, AssetsToCook);


	LOGINFO("HalfPipeApp shutdown");
}