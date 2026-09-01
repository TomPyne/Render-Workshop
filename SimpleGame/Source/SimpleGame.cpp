#include "SimpleGameApp.h"

#include <Core/WindowsPlatform.h>
#include <Shared/FileUtils/PathUtils.h>

int main()
{
	Path_s::SetDefaultProject(L"SimpleGame");

    GApp = new SimpleGameApp_c();

    WindowsPlatformMain("Simple Game", 1280, 800);

    delete GApp;
    return 0;
}