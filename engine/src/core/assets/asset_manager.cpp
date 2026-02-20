#include "core/assets/asset_manager.h"
#include "core/assets/types/model.h"

#include "core/allocators.h"
#include "core/filesystem.h"
#include "core/assets/asset_registry.h"
#include "core/assets/asset_serializer.h"

void IAsset::serializeName(ic::Serializer *serializer) const
{
        serializer->write<uint16_t>(m_name);
}

namespace ic
{

AssetManager *AssetManager::s_instance = nullptr;

void AssetManager::Initialize(const char *registryFile)
{
        if (!s_instance)
        {
                s_instance = new AssetManager();
                if (!s_instance->registry_.init(registryFile))
                        IC_CORE_ERROR("Failed to initialize registry");
        }
}

void AssetManager::Shutdown()
{
        delete s_instance;
        s_instance = nullptr;
}

AssetManager *AssetManager::Get()
{
        return s_instance;
}

AssetManager::~AssetManager()
{
        for (auto [k, v] : assets_)
        {
                if (v->isLoaded())
                {
                        unload(k);
                }
        }
}

IAsset *AssetManager::load(GUID id)
{
        // Already loaded?
        auto it = assets_.find(id);
        if (it != assets_.end())
        {
                it->second->addRef();
                return it->second;
        }

        if (!registry_.contains(id))
        {
                IC_CORE_WARN("Asset {} not found in registry", id);
                return nullptr;
        }

        AssetType type   = registry_.getAssetType(id);
        const char *path = registry_.getFilePath(id);

        IAsset *asset    = createAsset(type, id);
        if (!asset)
        {
                IC_CORE_ERROR("Unsupported asset type for {}", id);
                return nullptr;
        }

        if (!asset->load(path))
        {
                IC_CORE_ERROR("Failed to load asset {} from {}", id, path);
                return nullptr;
        }

        asset->addRef();

        IAsset *raw = asset;
        assets_[id] = asset;

        if (assets_.empty())
        {
                IC_CORE_WARN("Assets not registered event though loaded!");
        }

        IC_CORE_INFO("Loaded asset {}", id);

        return raw;
}

void AssetManager::unload(GUID id)
{
        auto it = assets_.find(id);
        if (it == assets_.end())
                return;

        if (it->second->release())
        {
                IC_CORE_INFO("Destroying asset {}", id);
                it->second->~IAsset();
                ic_free(it->second);
                assets_.erase(it);
        }
}

bool AssetManager::loadRegistry(const char *registry_file)
{
        return registry_.init(registry_file);
}

IAsset *AssetManager::createAsset(AssetType type, GUID id)
{
        // MEM MANAGEMENT OF THIS CLASS
        void *asset_memory = NULL;
        switch (type)
        {
        case AssetType::ASSET_TYPE_MODEL:
                asset_memory = ic_malloc(sizeof(Model));
                return new (asset_memory) Model(id);

                // case AssetType::ASSET_TYPE_TEXTURE:
                //     return new Texture(id);

                // case AssetType::ASSET_TYPE_MATERIAL:
                //     return new Material(id);

                // case AssetType::ASSET_TYPE_SHADER:
                //     return new Shader(id);

        default:
                return nullptr;
        }
}

AssetRegistry *AssetManager::getRegistry()
{
        return &registry_;  // Instead we can return only the vector to the registry
}

}  // namespace ic

void ic_load_registry(const char *registry_file_path)
{
        if (!ic::AssetManager::Get()->getRegistry()->init(registry_file_path))
        {
                IC_CORE_ERROR("Could not load registry: {}", registry_file_path);
        }
}

// remove this ic here somehow
Model *ic_load_model(GUID modelID)
{
        IC_CORE_INFO("Loading model with ID: {}", modelID);
        return ic::AssetManager::Get()->loadAs<Model>(modelID);
}

bool ic_render_model(GUID modelID, float *transform4x4)
{
        return false;
}

void ic_unload_model(GUID modelID)
{
        ic::AssetManager::Get()->unload(modelID);
        IC_CORE_INFO("Unloaded model with ID {}", modelID);
}
