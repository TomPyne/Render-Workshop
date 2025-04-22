#pragma once

#include <cstdint>

#include <Render/RenderTypes.h>

rl::RenderInitParams GetAppRenderParams();

bool InitializeApp();

void ResizeApp(uint32_t width, uint32_t height);

void Update(float deltaSeconds);

void ImguiUpdate();

void Render(rl::RenderView* view, rl::CommandListSubmissionGroup* clGroup, float deltaSeconds);

void ShutdownApp();