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

        bool           loadRegistry(const char *registry_file);
        AssetRegistry *getRegistry();

        IAsset *load(GUID id);

        template <typename T>
        T *loadAs(GUID id)
        {
                // THIS DYNAMIC CAST DOESNT WORK
                return (T *)(load(id));
        }

        template <typename T>
        T *loadFromPath(const char *filepath)
        {
                GUID id;  // TODO: make this work somehow
                return (T *)(load(id));
        }

        template <typename T>
        T *getAsset(GUID id)
        {
                auto it = assets_.find(id);
                if (it == assets_.end())
                {
                        IC_CORE_ERROR("Could not find the asset requested for ID: {}", id);
                        return nullptr;
                }

                return (T *)assets_[id];
        }

        void unload(GUID id);

        using InternalIterator = std::unordered_map<GUID, IAsset *>::iterator;
        using Iterator         = MapIterator<InternalIterator, GUID, IAsset *>;

        Iterator begin() { return Iterator(assets_.begin()); }
        Iterator end() { return Iterator(assets_.end()); }

        ~AssetManager();

private:
        static AssetManager *s_instance;

        AssetManager() = default;

        std::unordered_map<GUID, IAsset *> assets_;
        AssetRegistry                      registry_;

        IAsset *createAsset(AssetType type, GUID id);
};

}  // namespace ic

#ifdef __cplusplus
extern "C"
{
#endif
        typedef class Model Model;

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
        IC_API Model *ic_load_model(GUID modelId);

        IC_API const char *ic_get_model_path(GUID modelID);

        IC_API bool ic_render_model(GUID modelID, float *transform4x4);

        /** @function ic_unload_model
         * @category assets
         * @brief Unloads a model with a given GUID removes one instance if it goes below zero releases its memory as
         * well
         * @param modelID model id from the registry file.
         */
        IC_API void ic_unload_model(GUID modelId);

#ifdef __cplusplus
}
#endif

#endif