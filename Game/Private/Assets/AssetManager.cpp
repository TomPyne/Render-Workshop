#include "Assets/AssetManager.h"

#include "Assets/MaterialManager.h"
#include "Assets/MeshManager.h"
#include "Assets/TextureManager.h"
#include "Core/GameApp.h"
#include "Rendering/Materials.h"
#include "Rendering/Mesh.h"
#include "Rendering/Texture.h"

#include <Shared/FileUtils/PathUtils.h>
#include <Shared/Logging/Logging.h>

void AssetManager_c::Init()
{
    DefaultMaterial = MaterialManager::RequestMaterialInstance(Path_s(PathDirectory_e::Assets, L"Game", L"Materials/DefaultBRDF.hp_mtl"));
    ENSUREMSG(DefaultMaterial != nullptr, "[AssetManager_c::Init] Failed to load default material");
}

std::shared_ptr<Mesh_s> AssetManager_c::TryGetMesh(uint64_t Hash)
{
    auto& LoadedMeshes = Get().LoadedMeshes;
    auto It = LoadedMeshes.find(Hash);
    if (It != LoadedMeshes.end())
    {
        return It->second;
    }
    return nullptr;
}

void AssetManager_c::CacheMesh(uint64_t Hash, const std::shared_ptr<Mesh_s>& Mesh)
{
    Get().LoadedMeshes[Hash] = Mesh;

    Get().MeshesQueuedForRTBuild.push_back(Mesh);
}

std::shared_ptr<MaterialShaderInstance_c> AssetManager_c::TryGetMaterialInstance(uint64_t Hash)
{
    auto& LoadedMaterialInstances = Get().LoadedMaterialInstances;
    auto It = LoadedMaterialInstances.find(Hash);
    if (It != LoadedMaterialInstances.end())
    {
        return It->second;
    }
    return nullptr;
}

void AssetManager_c::CacheMaterialInstance(uint64_t Hash, const std::shared_ptr<MaterialShaderInstance_c>& MaterialInstance)
{
    Get().LoadedMaterialInstances[Hash] = MaterialInstance;
}

std::shared_ptr<Texture_s> AssetManager_c::TryGetTexture(uint64_t Hash)
{
    auto& LoadedTextures = Get().LoadedTextures;
    auto It = LoadedTextures.find(Hash);
    if (It != LoadedTextures.end())
    {
        return It->second;
    }
    return nullptr;
}

void AssetManager_c::CacheTexture(uint64_t Hash, const std::shared_ptr<Texture_s>& Texture)
{
    Get().LoadedTextures[Hash] = Texture;
}

std::shared_ptr<MaterialShaderInstance_c> AssetManager_c::GetDefaultMaterial()
{
    return AssetManager_c::Get().DefaultMaterial;
}

AssetManager_c& AssetManager_c::Get()
{
    AssetManager_c* AssetManager = GApp->GetAssetManager();
    ASSERTMSG(AssetManager, "[AssetManager_c::Get] GameApp has not initialized the asset manager");
    return *AssetManager;
}

void AssetManager_c::CollectMeshesForRTBuild(std::vector<Mesh_s*>& MeshesToBuild)
{
    MeshesToBuild.reserve(MeshesQueuedForRTBuild.size());
    for (const std::weak_ptr<Mesh_s>& QueuedMesh : MeshesQueuedForRTBuild)
    {
        if (QueuedMesh.expired())
            continue;

        MeshesToBuild.push_back(QueuedMesh.lock().get());
    }

    MeshesQueuedForRTBuild.clear();
}
