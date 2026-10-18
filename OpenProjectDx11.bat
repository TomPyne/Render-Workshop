@echo off
cd /d "%~dp0"

cmake --preset dx11 || exit /b 1
start "" "build\dx11\RenderWorkshopDx11.sln"
