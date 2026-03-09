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
        static void          Initialize(const char *registryFile);
        static void          Shutdown();
        static AssetManager *Get();

        // Non-copyable singleton
        AssetManager(const AssetManager &)            = delete;
        AssetManager &operator=(const AssetManager &) = delete;

        ~AssetManager();

        /**
         * Load by ID. Asset must already be in the registry with a filepath.
         * If already loaded, just increments refcount and returns cached pointer.
         * Returns nullptr if the ID is unknown or load fails.
         */
        IAsset *load(GUID id);

        /**
         * Load by ID + explicit filepath.
         * Registers the asset if it isn't already in the registry.
         * If already loaded, increments refcount and returns cached pointer.
         * path is used for the first load only — ignored on cache hits.
         */
        IAsset *load(GUID id, const char *path, AssetType type = ASSET_TYPE_MODEL);

        template <typename T>
        T *loadAs(GUID id)
        {
                IAsset *asset = load(id);
                if (!asset)
                        return nullptr;
                if (asset->getAssetType() != T::getStaticType())
                {
                        IC_CORE_ERROR("AssetManager::loadAs — type mismatch for {}", id);
                        unload(id);  // undo the addRef from load()
                        return nullptr;
                }
                return static_cast<T *>(asset);
        }

        template <typename T>
        T *loadAs(GUID id, const char *path)
        {
                IAsset *asset = load(id, path, T::getStaticType());
                if (!asset)
                        return nullptr;
                if (asset->getAssetType() != T::getStaticType())
                {
                        IC_CORE_ERROR("AssetManager::loadAs — type mismatch for {}", id);
                        unload(id);
                        return nullptr;
                }
                return static_cast<T *>(asset);
        }

        IAsset *getAsset(GUID id);

        template <typename T>
        T *getAsset(GUID id)
        {
                IAsset *asset = getAsset(id);
                if (!asset)
                        return nullptr;
                if (asset->getAssetType() != T::getStaticType())
                        return nullptr;
                return static_cast<T *>(asset);
        }

        void unload(GUID id);

        using InternalIterator = std::unordered_map<GUID, IAsset *>::iterator;
        using Iterator         = MapIterator<InternalIterator, GUID, IAsset *>;

        Iterator begin() { return Iterator(m_assets.begin()); }
        Iterator end() { return Iterator(m_assets.end()); }

        bool           isLoaded(GUID id) const;
        AssetRegistry *getRegistry() { return &m_registry; }
        bool           loadRegistry(const char *path);

private:
        AssetManager() = default;

        /** Allocate and construct an asset of the given type. */
        IAsset *createAsset(AssetType type, GUID id);

        /** Internal destroy — calls destructor + ic_free. */
        void destroyAsset(IAsset *asset);

        static AssetManager *s_instance;

        std::unordered_map<GUID, IAsset *> m_assets;
        AssetRegistry                      m_registry;
};

}  // namespace ic

#ifdef __cplusplus
extern "C"
{
#endif
        /** @function ic_load_registry
         * @category assets
         * @brief Loads the registy file from a file path. The registry file is (__/YAML/XML) file which contains
         * modelIDs and mapping of different files using GUIDs to filepaths.
         * @param registryFilePath path to the registry file. Loaded using filesystem module usually using mounting.
         */
        IC_API void ic_load_registry(const char *registryFilePath);

        /** @function ic_load_model
         * @category assets
         * @brief Loads models from asset id. Checks in the registry if it contains the GUID else returns
         * (exception/nothing) for now.
         elID modelID from a registry file.
         */
        IC_API bool ic_load_model(GUID modelId);

        IC_API const char *ic_get_model_path(GUID modelID);

        /** @function ic_unload_model
         * @category assets
         * @brief Unloads a model with a given GUID removes one instance if it goes below zero releases its memory as
         * well
         * @param modelID model id from the registry file.
         */
        IC_API bool ic_unload_model(GUID modelId);

#ifdef __cplusplus
}
#endif

#endif