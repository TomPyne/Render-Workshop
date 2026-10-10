#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

class MaterialShaderInstance_c;

struct Mesh_s;
struct Texture_s;

class AssetManager_c
{
public:
	void Init();

	static std::shared_ptr<Mesh_s> TryGetMesh(uint64_t Hash);
	static void CacheMesh(uint64_t Hash, const std::shared_ptr<Mesh_s>& Mesh);
	static void GetLoadedMeshes(std::vector<std::shared_ptr<Mesh_s>>& OutMeshes);

	static std::shared_ptr<MaterialShaderInstance_c> TryGetMaterialInstance(uint64_t Hash);
	static void CacheMaterialInstance(uint64_t Hash, const std::shared_ptr<MaterialShaderInstance_c>& MaterialInstance);

	static std::shared_ptr<Texture_s> TryGetTexture(uint64_t Hash);
	static void CacheTexture(uint64_t Hash, const std::shared_ptr<Texture_s>& Texture);

	static std::shared_ptr<MaterialShaderInstance_c> GetDefaultMaterial();

	static AssetManager_c& Get();

	// Consumes internal queue and returns a list of raw pointers to meshes requiring a build
	std::vector<const Mesh_s*> CollectMeshesForRTBuild();

private:

	std::unordered_map<uint64_t, std::shared_ptr<Mesh_s>> LoadedMeshes;
	std::unordered_map<uint64_t, std::shared_ptr<Texture_s>> LoadedTextures;
	std::unordered_map<uint64_t, std::shared_ptr<MaterialShaderInstance_c>> LoadedMaterialInstances;

	std::shared_ptr<MaterialShaderInstance_c> DefaultMaterial;

	std::vector<std::weak_ptr<Mesh_s>> MeshesQueuedForRTBuild;
};