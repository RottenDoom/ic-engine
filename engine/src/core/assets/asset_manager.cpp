#include "core/assets/asset_manager.h"
#include "core/assets/types/model.h"

#include "core/allocators.h"
#include "core/filesystem.h"
#include "core/assets/asset_registry.h"
#include "core/assets/asset_serializer.h"

namespace ic
{

static size_t assetSize(AssetType type)
{
        switch (type)
        {
        case ASSET_TYPE_MODEL:
                return sizeof(Model);
        // case ASSET_TYPE_TEXTURE:  return sizeof(Texture);
        // case ASSET_TYPE_MATERIAL: return sizeof(Material);
        // case ASSET_TYPE_SHADER:   return sizeof(ShaderAsset);
        default:
                IC_CORE_ERROR("assetSize: unknown AssetType {}", static_cast<int>(type));
                return 0;
        }
}

AssetManager *AssetManager::s_instance = nullptr;

void AssetManager::Initialize(const char *registryFile)
{
        IC_CORE_ASSERT(!s_instance, "AssetManager::Initialize called twice");
        s_instance = new AssetManager();

        if (registryFile && !s_instance->m_registry.init(registryFile))
                IC_CORE_WARN("AssetManager: failed to load registry '{}'", registryFile);
}

void AssetManager::Shutdown()
{
        delete s_instance;
        s_instance = nullptr;
}

AssetManager *AssetManager::Get()
{
        IC_CORE_ASSERT(s_instance, "AssetManager not initialized - call Initialize() first");
        return s_instance;
}

AssetManager::~AssetManager()
{
        // Force-destroy all remaining assets regardless of refcount.
        // This is the shutdown path - no callers remain.
        for (auto &[id, asset] : m_assets)
        {
                IC_CORE_TRACE("AssetManager: force-destroying asset {} at shutdown", id);
                destroyAsset(asset);
        }
        m_assets.clear();
}

IAsset *AssetManager::load(GUID id)
{
        // --- Cache hit: already loaded, just bump refcount ---
        auto it = m_assets.find(id);
        if (it != m_assets.end())
        {
                it->second->addRef();
                IC_CORE_TRACE("AssetManager: cache hit for {} (refCount={})", id, it->second->getRefNum());
                return it->second;
        }

        // --- Cache miss: must be in registry ---
        if (!m_registry.contains(id))
        {
                IC_CORE_WARN("AssetManager::load - asset {} not in registry", id);
                return nullptr;
        }

        const char *path = m_registry.getFilePath(id);
        AssetType   type = m_registry.getAssetType(id);

        return load(id, path, type);
}

IAsset *AssetManager::load(GUID id, const char *path, AssetType type)
{
        IC_CORE_ASSERT(path, "AssetManager::load - null path");

        // --- Cache hit ---
        auto it = m_assets.find(id);
        if (it != m_assets.end())
        {
                it->second->addRef();
                IC_CORE_TRACE("AssetManager: cache hit for {} (refCount={})", id, it->second->getRefNum());
                return it->second;
        }

        // --- Register if not already in registry ---
        if (!m_registry.contains(id))
        {
                m_registry.registerAsset(id, path, type);
        }

        // --- Construct ---
        IAsset *asset = createAsset(type, id);
        if (!asset)
        {
                IC_CORE_ERROR("AssetManager::load - unsupported asset type {} for id {}", static_cast<int>(type), id);
                return nullptr;
        }

        // --- Load from disk ---
        if (!asset->load(path))
        {
                IC_CORE_ERROR("AssetManager::load - failed to load '{}' (id={})", path, id);
                destroyAsset(asset);
                return nullptr;
        }

        // --- Cache and addRef ---
        asset->addRef();  // refCount = 1
        m_assets[id] = asset;

        IC_CORE_INFO("AssetManager: loaded '{}' (id={}, refCount=1)", path, id);
        return asset;
}

IAsset *AssetManager::getAsset(GUID id)
{
        auto it = m_assets.find(id);
        if (it == m_assets.end())
        {
                IC_CORE_WARN("AssetManager::getAsset - {} not loaded", id);
                return nullptr;
        }
        return it->second;
}

void AssetManager::unload(GUID id)
{
        auto it = m_assets.find(id);
        if (it == m_assets.end())
        {
                IC_CORE_WARN("AssetManager::unload - {} not in cache", id);
                return;
        }

        IAsset *asset = it->second;

        // release() decrements and returns true when refCount hits 0
        if (asset->release())
        {
                IC_CORE_INFO("AssetManager: destroying asset {} (refCount=0)", id);
                m_assets.erase(it);
                destroyAsset(asset);
        }
        else
        {
                IC_CORE_TRACE("AssetManager: unload {} - refCount now {}", id, asset->getRefNum());
        }
}

bool AssetManager::isLoaded(GUID id) const
{
        auto it = m_assets.find(id);
        return it != m_assets.end() && it->second->isLoaded();
}

bool AssetManager::loadRegistry(const char *path)
{
        return m_registry.init(path);
}

IAsset *AssetManager::createAsset(AssetType type, GUID id)
{
        void *mem = ic_malloc(assetSize(type));
        if (!mem)
        {
                IC_CORE_ERROR("AssetManager::createAsset - allocation failed for type {}", static_cast<int>(type));
                return nullptr;
        }

        switch (type)
        {
        case ASSET_TYPE_MODEL:
                return new (mem) Model(id);

                // Uncomment as new asset types are added:
                // case ASSET_TYPE_TEXTURE:  return new (mem) Texture(id);
                // case ASSET_TYPE_MATERIAL: return new (mem) Material(id);
                // case ASSET_TYPE_SHADER:   return new (mem) Shader(id);

        default:
                ic_free(mem);
                return nullptr;
        }
}

void AssetManager::destroyAsset(IAsset *asset)
{
        if (!asset)
                return;
        // Explicit destructor call required for placement-new objects.
        asset->~IAsset();
        ic_free(asset);
}

}  // namespace ic

void ic_load_registry(const char *registry_file_path)
{
        IC_CORE_ASSERT(registry_file_path, "ic_load_registry - null path");
        if (!ic::AssetManager::Get()->loadRegistry(registry_file_path))
                IC_CORE_ERROR("ic_load_registry - failed to load '{}'", registry_file_path);
}

bool ic_load_model(GUID modelID)
{
        IC_CORE_INFO("Loading model with ID: {}", modelID);
        if (!ic::AssetManager::Get()->loadAs<ic::Model>(modelID))
        {
                return false;
        }
        return true;
}

const char *ic_get_model_path(GUID modelID)
{
        ic::AssetRegistry *reg = ic::AssetManager::Get()->getRegistry();
        if (!reg->contains(modelID))
                return nullptr;
        return reg->getFilePath(modelID);
}

bool ic_unload_model(GUID modelID)
{
        ic::AssetManager::Get()->unload(modelID);
        return true;
}
