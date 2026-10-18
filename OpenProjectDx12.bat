@echo off
cd /d "%~dp0"

cmake --preset dx12 || exit /b 1
start "" "build\dx12\RenderWorkshopDx12.sln"
