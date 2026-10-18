@echo off
cd /d "%~dp0"

cmake --preset dx12 || exit /b 1
cmake --build --preset dx12-debug || exit /b 1
