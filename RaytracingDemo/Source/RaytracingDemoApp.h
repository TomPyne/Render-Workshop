#pragma once

#include <cstdint>

#include <Render/RenderTypes.h>

tpr::RenderInitParams GetAppRenderParams();

bool InitializeApp();

void ResizeApp(uint32_t width, uint32_t height);

void Update(float deltaSeconds);

void ImguiUpdate();

void Render(tpr::RenderView* view, tpr::CommandListSubmissionGroup* clGroup, float deltaSeconds);

void ShutdownApp();