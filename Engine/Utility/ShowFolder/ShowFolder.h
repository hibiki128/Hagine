#pragma once
#include <d3d12.h>
#include <string>
#include <unordered_map>

struct TextureCache {
    uint32_t srvIndex = 0;              // SRV index
    D3D12_GPU_DESCRIPTOR_HANDLE handle; // GPU handle
    int width = 0;
    int height = 0;
};

void ShowTextureFile(std::string &selectedTexturePath);
void ShowModelFile(std::string &selectedModelPath);
void ShowJsonFile(std::string &selectedJsonPath, std::string &startPath);
void ShowGltfFile(std::string &selectedGltfPath);

static std::unordered_map<std::string, TextureCache> textureCache;