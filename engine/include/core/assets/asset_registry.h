#ifndef ASSET_REGISTRY_H
#define ASSET_REGISTRY_H

#include "defines.h"
#include "core/assets/types/asset_base.h"

/** TODO:
 * 1. Write functions for writting assets registy and getting the file path to the current registry.
 */

namespace YAML
{
class Node;
}

namespace ic
{

/** AssetRegisty class responsible for physical database for directories parsed from registy files. */
class AssetRegistry
{
public:
        AssetRegistry();

        // Initialize a registy with registy yaml file
        bool init(const char *assets_registry_file);

        // Save the registry file in yaml format
        bool save(const char *assets_registry_file);

        // Check if an asset is in the registry
        bool contains(GUID id) const;

        // Return an asset's GUID based on the filepath. If filepath does not exist return INVALID_GUID
        GUID getAssetId(const char *file_path) const;

        AssetType getAssetType(GUID id) const;

        // Return file path for a asset id
        const char *getFilePath(GUID id) const;

        // Get relative file path of a file with respect to root mount folder.
        const char *getCachePath(GUID id) const;

        // Register filepath into the asset registy.
        GUID registerAsset(const char *file_path, AssetType type);
        void registerDependency(GUID id, GUID dependency_id);

        const std::unordered_set<GUID> *getDependencies(GUID id) const;

        void unregister(GUID id);

private:
        // common asset folder
        string assets_folder_;

        std::unordered_map<GUID, AssetMeta>                assets_;
        std::unordered_map<string, GUID>                   ids_;
        std::unordered_map<GUID, std::unordered_set<GUID>> dependencies_;

        void             parseAssetEntry(const YAML::Node &node);
        static AssetType assetTypeFromString(const string &s);
        static string    assetTypeToString(AssetType type);
};

}  // namespace ic

#endif