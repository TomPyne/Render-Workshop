#pragma once

struct Model_s;

bool LoadModelFromWavefront(const wchar_t* WavefrontPath, Model_s& OutModel);