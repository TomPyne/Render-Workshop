
#include <Game/Core/WindowsPlatform.h>
#include <Game/Core/GameApp.h>

int main()
{
    GameApp_c* App = new GameApp_c();

    WindowsPlatformMain("Simple Game", 1280, 800, App);

    delete App;
    return 0;
}