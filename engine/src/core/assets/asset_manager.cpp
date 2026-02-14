#include "core/assets/asset_manager.h"
#include "core/assets/types/model.h"

#include "core/allocators.h"
#include "core/filesystem.h"
#include "core/assets/asset_registry.h"
#include "core/assets/asset_serializer.h"

// global unique asset manager
ic::AssetManager *g_asset_manager = nullptr;
static GUID UUID                  = 1u;

void IAsset::serializeName(ic::Serializer *serializer) const
{
        serializer->write<uint16_t>(m_name);
}

namespace ic
{

void asset_manager_init(void)
{
        void *memory    = ic_malloc(sizeof(AssetManager));
        g_asset_manager = new (memory) AssetManager();  // constructor runs
        g_asset_manager->Init("/assets/registry.yaml");
}

void asset_manager_deinit(void)
{
        g_asset_manager->~AssetManager();
        ic_free(g_asset_manager);
}

void AssetManager::Init(const char *assets_registry_file)
{
        IC_CORE_ASSERT(assets_registry_file, "File does not exist: {}", assets_registry_file);
        if (!fs_exists(assets_registry_file))
        {
                /** TODO: Add default registry here by hand and save it as well */
                // SOME_FUNCTION_FOR_WRITING_YAML_TO_FILE("TEXT_YAML", registry_path + "registry.yaml");
                // SAVE(registry_file);
                // registry->Init(registry_file);
                IC_CORE_TRACE("TODO IMPLEMENT: {}, Line: {}", __FILE_NAME__, __LINE__);
        }

        if (!registry_.Init(assets_registry_file))
        {
                IC_CORE_ERROR("Could not initialize registry");
        }

        IC_CORE_TRACE("Initialized registry");

        // some kind of debug function that allows me to view all the data.
}

bool AssetManager::ReloadAsset(IAsset *asset)
{
        return false;
}

void AssetManager::ReleaseAsset(IAsset *asset) {}

bool AssetManager::LoadRegistry(const char *registry_file_path)
{
        // TO load the new registry the past assets need to be cleaned as well we will see about that later.
        return registry_.Init(registry_file_path);  // TODO fix this
}

AssetRegistry *AssetManager::GetRegistry()
{
        return &registry_;  // Instead we can return only the vector to the registry
}

template <typename T>
T *AssetManager::Get(GUID asset_id)
{
        return new T;
}

template <typename T>
T *AssetManager::Load(const char *file_path)
{
        return nullptr;
}
template <typename T>
void AssetManager::SerializeAsset(T *asset, const char *filename)
{
}
template <typename T>
bool AssetManager::AddSerializer(Serializer *serializer)
{
        return false;
}

}  // namespace ic

void ic_load_registry(const char *registry_file_path)
{
        if (!g_asset_manager->LoadRegistry(registry_file_path))
        {
                IC_CORE_ERROR("Could not load registry: {}", registry_file_path);
        }
}

// remove this ic here somehow
Model *ic_load_model(GUID modelID)
{
        // check if the registry contains the model
        if (!g_asset_manager->GetRegistry()->Contains(modelID))
        {
                IC_CORE_WARN("Model ID {} not in registry!", modelID);
                return nullptr;
        }

        // either load the model or return the pointer if already loaded
        Model *cached = g_asset_manager->Get<Model>(modelID);
        if (cached)
        {
                cached->AddRef();
                IC_CORE_INFO("Model {} already loaded, refcount: {}", modelID, cached->GetRefNum());
                return cached;
        }

        // if not cached load from file path for now
        const char *filepath = g_asset_manager->GetRegistry()->GetFilePath(modelID);
        IC_CORE_INFO("Loading model from: {}", filepath);

        g_asset_manager->GetRegistry()->Register(filepath);  // modelID and model should go in here.
                                                             // model->addRef()

        ic::Serializer serializer;
        if (!cached->CachedLoad(&serializer))
        {
                IC_CORE_INFO("Could not load fast file");
        }
        else
        {
                IC_CORE_ASSERT(cached->Load(filepath), "Could not load file!");
                cached->CachedSave(&serializer);
        }

        // THis means the model was't loaded yet and thus we load the model by initializing asset class adding to
        // assetmanager(reposibility of asset manager ofcourse) increase refcounts and the loaded boolean. But this
        // model might share the same model ptr internally but might have different internal ids for distinction based
        // on locations and model matrices.

        // next task after this would be to make this process multithreaded or streamable.

        IC_CORE_INFO("Loaded model with ID {}", modelID);
        return nullptr; /** TODO: COMPlET THIS FUNCTION */
}

void ic_unload_model(GUID modelID)
{
        // Get the cached asset
        // Model* model = g_asset_manager->GetLoaded<Model>(modelID);
        // if (!model)
        // {
        //         IC_CORE_WARN("Trying to unload model {} that isn't loaded", modelID);
        //         return;
        // }

        // // Decrement refcount
        // int32_t new_refcount = model->Release();

        // IC_CORE_INFO("Model {} refcount: {}", modelID, new_refcount);

        // // If refcount reaches 0, actually unload
        // if (new_refcount == 0)
        // {
        //         g_asset_manager->UnloadAsset(modelID);
        //         delete model;  // Free memory
        //         IC_CORE_INFO("Model {} fully unloaded", modelID);
        // }
        IC_CORE_INFO("Unloaded model with ID {}", modelID);
}
