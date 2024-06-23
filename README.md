# Render-Demos

This repo provides 2 basic examples of how to use the tpr render lib.

**RenderExample** - This example generates and rotates a cube in front of the camera

![image](https://github.com/TomPyne/Render-Demos/assets/13146584/6d5904c3-6122-4208-be5b-ee79a2d16e80)

**RenderExampleImGui** - This example implements the ImGui docking branch

![image](https://github.com/TomPyne/Render-Demos/assets/13146584/781394c5-1e52-4d67-b68d-0ffbfcc1d67b)

Both of these examples come with a Dx11 and Dx12 version demonstrating how both work. 

## Prerequisites
- cmake version 3.10+
- Visual Studio 2022+

## Setup

This repository uses submodules so you must clone with `git clone --recurse-submodules`

Run BuildProject.bat

- The solution can be found inside the generated `build` directory.
- Compiled binaries are exported to the `bin` folder.
- Compiled libraries are exported to the `lib` folder.

Open the solution and select any of the render example projects as your start up project and run.
