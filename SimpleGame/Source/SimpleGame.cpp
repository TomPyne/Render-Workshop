#include "SimpleGameApp.h"

#include <Core/WindowsPlatform.h>
#include <Shared/FileUtils/PathUtils.h>

int main()
{
	Path_s::SetDefaultProject(L"SimpleGame");

    SimpleGameApp_c* App = new SimpleGameApp_c();

    WindowsPlatformMain("Simple Game", 1280, 800, App);

    delete App;
    return 0;
}