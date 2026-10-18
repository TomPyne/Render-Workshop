@echo off
cd /d "%~dp0"

cmake --preset dx11 || exit /b 1
cmake --build --preset dx11-debug || exit /b 1
