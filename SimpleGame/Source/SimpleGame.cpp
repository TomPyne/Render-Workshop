#include "SimpleGameApp.h"

#include <Game/Core/WindowsPlatform.h>

int main()
{
    SimpleGameApp_c* App = new SimpleGameApp_c();

    WindowsPlatformMain("Simple Game", 1280, 800, App);

    delete App;
    return 0;
}