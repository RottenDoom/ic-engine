#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include "defines.h"
#include "core/assets/types/asset_base.h"
#include "core/assets/asset_registry.h"
#include "core/assets/asset_serializer.h"

/**
 * TODO:
 * 1. Implement and test all the functions.
 * 2. Write an asset parser that parses gltf to internal binary asset types (deserializer + parser + emmitter)
 * 3. Define the workflow somewhere.
 * 4. Fix the filesystem with some tests
 * 5. Add more functions that might be used internally or externally
 * 6. Fix the formatting to be consistent.
 */

namespace ic
{

void asset_manager_init(void);
void asset_manager_deinit(void);

class AssetManager
{
public:
        AssetManager() {}

        void Init(const char *assets_registry_file);

        /**
         * @brief Checks if the asset is already loaded if its loaded returns it else returns nullptr
         */
        template <typename T>
        T *Get(GUID asset_id);

        /**
         * @param file_path File path relative to the assets folder.
         */
        template <typename T>
        T *Load(const char *file_path);

        bool ReloadAsset(IAsset *asset);

        /**
         * @brief Decrements the ref count of the asset and if it reaches 0 unloads the asset.
         */
        void ReleaseAsset(IAsset *asset);

        /**
         * @brief Serializes the asset to file.
         * @param filename File path NOT relative to the assets folder.
         */
        template <typename T>
        void SerializeAsset(T *asset, const char *filename);

        bool LoadRegistry(const char *registry_file_path);
        AssetRegistry *GetRegistry();

        template <typename T>
        bool AddSerializer(Serializer *serializer);

private:
        AssetRegistry registry_;

        std::unordered_map<GUID, IAsset *> assets_;                      // THIS TOO;
        std::unordered_map<AssetType, Serializer *> asset_serializers_;  // REPLACE THIS SHIT
};

}  // namespace ic

/** TODO design dark souls type game from scratch and make it work with a certain type of asset manager and then
 * abstract it. */
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
         Model Model *ic_load_model(GUID modelId);

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