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
	s_instance->m_AssetRegistryPath = registryFile;

        if (registryFile && !s_instance->m_registry.Init(registryFile))
                IC_CORE_WARN("AssetManager: failed to load registry '{}'", registryFile);
}

void AssetManager::Shutdown()
{
	if (AssetManager::Get().GetRegistry()->IsUnsaved())
	{
		AssetManager::Get().GetRegistry()->Save(s_instance->m_AssetRegistryPath.c_str());
	}
        delete s_instance;
        s_instance = nullptr;
}

AssetManager::~AssetManager()
{
        // Force-destroy all remaining assets regardless of refcount.
        // This is the shutdown path - no callers remain.
        for (auto &[id, asset] : m_assets)
        {
                IC_CORE_TRACE("AssetManager: force-destroying asset {} at shutdown", id);
                DestroyAsset(asset);
        }
        m_assets.clear();
}

IAsset *AssetManager::Load(IC_GUID id)
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
        if (!m_registry.Contains(id))
        {
                IC_CORE_WARN("AssetManager::load - asset {} not in registry", id);
                return nullptr;
        }

        const char *path = m_registry.GetFilePath(id);
        AssetType   type = m_registry.GetAssetType(id);

        return Load(id, path, type);
}

IAsset *AssetManager::Load(IC_GUID id, const char *path, AssetType type)
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
        if (!m_registry.Contains(id))
        {
                m_registry.RegisterAsset(id, path, type);
        }

        // --- Construct ---
        IAsset *asset = CreateAsset(type, id);
        if (!asset)
        {
                IC_CORE_ERROR("AssetManager::load - unsupported asset type {} for id {}", static_cast<int>(type), id);
                return nullptr;
        }

        // --- Load from disk ---
        if (!asset->load(path))
        {
                IC_CORE_ERROR("AssetManager::load - failed to load '{}' (id={})", path, id);
                DestroyAsset(asset);
                return nullptr;
        }

        // --- Cache and addRef ---
        asset->addRef();  // refCount = 1
        m_assets[id] = asset;

        IC_CORE_INFO("AssetManager: loaded '{}' (id={}, refCount=1)", path, id);
        return asset;
}

IAsset *AssetManager::GetAsset(IC_GUID id)
{
        auto it = m_assets.find(id);
        if (it == m_assets.end())
        {
                IC_CORE_WARN("AssetManager::getAsset - {} not loaded", id);
                return nullptr;
        }
        return it->second;
}

void AssetManager::Unload(IC_GUID id)
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
                DestroyAsset(asset);
        }
        else
        {
                IC_CORE_TRACE("AssetManager: unload {} - refCount now {}", id, asset->getRefNum());
        }
}

bool AssetManager::IsLoaded(IC_GUID id) const
{
        auto it = m_assets.find(id);
        return it != m_assets.end() && it->second->isLoaded();
}

bool AssetManager::LoadRegistry(const char *path)
{
        return m_registry.Init(path);
}

IAsset *AssetManager::CreateAsset(AssetType type, IC_GUID id)
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

void AssetManager::DestroyAsset(IAsset *asset)
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
        if (!ic::AssetManager::Get().LoadRegistry(registry_file_path))
                IC_CORE_ERROR("ic_load_registry - failed to load '{}'", registry_file_path);
}

IC_GUID ic_load_model(const char* name)
{
	IC_GUID modelID = ic::AssetManager::Get().GetRegistry()->GetAssetId(name);
	if (modelID == INVALID_ID) {
		IC_CORE_ERROR("Model {} does not exist!", name);
		return modelID;
	}
        IC_CORE_INFO("Loading model with ID: {}", modelID);

	/** TODO maybe make a direct named model loading */
        if (!ic::AssetManager::Get().LoadAs<ic::Model>(modelID))
        {
		IC_CORE_WARN("Could not load model correctly!");
                return modelID;
        }
        return modelID;
}

void ic_name_model(IC_GUID id, const char* name) {
	ic::AssetRegistry* reg = ic::AssetManager::Get().GetRegistry();
	reg->SetAssetName(id, name);
}

const char *ic_get_model_path(IC_GUID modelID)
{
        ic::AssetRegistry *reg = ic::AssetManager::Get().GetRegistry();
        if (!reg->Contains(modelID))
                return nullptr;
        return reg->GetFilePath(modelID);
}

bool ic_unload_model(IC_GUID modelID)
{
        ic::AssetManager::Get().Unload(modelID);
        return true;
}
