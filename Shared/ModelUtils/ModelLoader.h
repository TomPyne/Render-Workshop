#pragma once

struct ModelAsset_s;

bool LoadModelFromWavefront(const wchar_t* WavefrontPath, ModelAsset_s& OutModel);