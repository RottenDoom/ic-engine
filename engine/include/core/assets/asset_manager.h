#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include "defines.h"
#include "core/assets/types/asset_base.h"
#include "core/assets/asset_registry.h"
#include "core/iterator.h"

namespace ic
{

class Serializer;

class AssetManager
{
public:
        /** Registry initialization of the asset. The registry should be full path or filesystem mounted path */
        static void Initialize(const char *registryFile);

        /** Assetmanager shut down */
        static void          Shutdown();
        static AssetManager &Get() { return *s_instance; }

        // Non-copyable singleton
        AssetManager(const AssetManager &)            = delete;
        AssetManager &operator=(const AssetManager &) = delete;

        ~AssetManager();

        /**
         * Insert an already constructed asset the manager will own and later destroy.
         * Used for asset with no backing file (glTF, editor created materials). Sets refcounts += 1.
         */
        IAsset *AddRuntimeAsset(IAsset *asset);

        /** Load an asset with explicit type and id */
        template <typename T>
        T *LoadAs(IC_GUID id)
        {
                IAsset *asset = Load(id);
                if (!asset)
                        return nullptr;
                if (asset->getAssetType() != T::getStaticType())
                {
                        IC_CORE_ERROR("AssetManager::LoadAs - type mismatch for {}", id);
                        Unload(id);  // undo the addRef from load()
                        return nullptr;
                }
                return static_cast<T *>(asset);
        }

        /** Load an asset with a path and id. If the asset does not exist in registry registers it*/
        template <typename T>
        T *LoadAs(IC_GUID id, const char *path)
        {
                IAsset *asset = Load(id, path, T::getStaticType());
                if (!asset)
                        return nullptr;
                if (asset->getAssetType() != T::getStaticType())
                {
                        IC_CORE_ERROR("AssetManager::LoadAs - type mismatch for {}", id);
                        Unload(id);
                        return nullptr;
                }
                return static_cast<T *>(asset);
        }

        /** Load from asset name if it exists in the registry */
        template <typename T>
        T *LoadAs(const char *name)
        {
                IC_GUID     id   = m_registry.GetAssetId(name);
                const char *path = m_registry.GetFilePath(id);
                if (id != INVALID_ID && path)
                {
                        IAsset *asset = Load(id, path, T::getStaticType());
                        if (!asset)
                                return nullptr;
                        if (asset->getAssetType() != T::getStaticType())
                        {
                                IC_CORE_ERROR("AssetManager::LoadAs - type mismatch for {}", id);
                                Unload(id);
                                return nullptr;
                        }
                        return static_cast<T *>(asset);
                }
                else
                {
                        IC_CORE_ERROR("AssetManager::LoadAs - Asset ID or path does not exist in registry!");
                        return nullptr;
                }
        }

        /** Returns asset without incrementing the ref count */
        template <typename T>
        T *GetAsset(IC_GUID id)
        {
                IAsset *asset = GetAsset(id);
                if (!asset)
                        return nullptr;
                if (asset->getAssetType() != T::getStaticType())
                        return nullptr;
                return static_cast<T *>(asset);
        }

        /** Unloads the asset with the given destroys the asset if its ref count is zero*/
        void Unload(IC_GUID id);

        using InternalIterator = std::unordered_map<IC_GUID, IAsset *>::iterator;
        using Iterator         = MapIterator<InternalIterator, IC_GUID, IAsset *>;

        Iterator begin() { return Iterator(m_assets.begin()); }
        Iterator end() { return Iterator(m_assets.end()); }

        bool           IsLoaded(IC_GUID id) const;
        AssetRegistry *GetRegistry() { return &m_registry; }
        bool           LoadRegistry(const char *path);

private:
        /**
         * Load by ID. Asset must already be in the registry with a filepath.
         * If already loaded, just increments refcount and returns cached pointer.
         * Returns nullptr if the ID is unknown or load fails.
         */
        IAsset *Load(IC_GUID id);

        /**
         * Load by ID + explicit filepath.
         * Registers the asset if it isn't already in the registry.
         * If already loaded, increments refcount and returns cached pointer.
         * path is used for the first load only - ignored on cache hits.
         */
        IAsset *Load(IC_GUID id, const char *path, AssetType type = ASSET_TYPE_MODEL);

        /** Gets asset from an id */
        IAsset *GetAsset(IC_GUID id);

        AssetManager() = default;

        /** Allocate and construct an asset of the given type. */
        IAsset *CreateAsset(AssetType type, IC_GUID id);

        /** Internal destroy - calls destructor + ic_free. */
        void DestroyAsset(IAsset *asset);

        /** Asset manager singleton. This can be accessed by anywhere by the user */
        static AssetManager *s_instance;

        /** Registry path for initializing registry for the asset manager */
        string m_AssetRegistryPath;

        /** Map of ids to asset references. TODO: tackle circular references */
        std::unordered_map<IC_GUID, IAsset *> m_assets;

        /** Registry is owned by the asset manager singleton */
        AssetRegistry m_registry;
};

}  // namespace ic

#ifdef __cplusplus
extern "C"
{
#endif
        /** @function ic_load_registry
         * @category assets
         * @brief Loads the registy file from a file path. The registry file is (__/YAML/XML) file which contains
         * modelIDs and mapping of different files using IC_GUIDs to filepaths.
         * @param registryFilePath path to the registry file. Loaded using filesystem module usually using mounting.
         */
        IC_API void ic_load_registry(const char *registryFilePath);

        /** @function ic_load_model
         * @category assets
         * @brief Loads a model using model name. Returns false if no model of that name found.
         elID modelID from a registry file.
         */
        IC_API IC_GUID ic_load_model(const char *name);

        // TODO: load model without registry and then save it into the file.

        IC_API void ic_name_model(IC_GUID id, const char *name);

        IC_API const char *ic_get_model_path(IC_GUID modelID);

        /** @function ic_unload_model
         * @category assets
         * @brief Unloads a model with a given IC_GUID removes one instance if it goes below zero releases its memory as
         * well
         * @param modelID model id from the registry file.
         */
        IC_API bool ic_unload_model(IC_GUID modelId);

#ifdef __cplusplus
}
#endif

#endif  // ASSET_MANAGER