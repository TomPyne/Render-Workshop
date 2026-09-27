#include "SunTempleApp.h"

#include <Core/WindowsPlatform.h>
#include <Shared/FileUtils/PathUtils.h>

int main()
{
	Path_s::SetDefaultProject(L"SunTemple");

    GApp = new SunTempleApp_c();

    WindowsPlatformMain("Sun Temple", 1280, 800);

    delete GApp;
    return 0;
}