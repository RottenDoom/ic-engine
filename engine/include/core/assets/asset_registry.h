#ifndef ASSET_REGISTRY_H
#define ASSET_REGISTRY_H

#include "defines.h"
#include "core/assets/types/asset_base.h"

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
        bool Init(const char *assets_registry_file);

        // Save the registry file in yaml format
        bool Save(const char *assets_registry_file);

        // Check if an asset is in the registry
        bool Contains(GUID id) const;

        // Return an asset's GUID based on the filepath. If filepath does not exist return INVALID_GUID
        GUID GetAssetId(const char *file_path) const;

        // Return file path for a asset id
        const char *GetFilePath(GUID id) const;

        // Get relative file path of a file with respect to root mount folder.
        char *GetRelFilePath(GUID id) const;

        // Register filepath into the asset registy.
        GUID Register(const char *file_path);
        void RegisterDependency(GUID id, GUID dependency_id);

        const std::unordered_set<GUID> *GetDependencies(GUID id) const;

        void Unregister(GUID id);

private:
        // common asset folder
        string assets_folder_;

        std::unordered_map<GUID, const char *> file_paths_;
        std::unordered_map<const char *, GUID> ids_;
        std::unordered_map<GUID, std::unordered_set<GUID>> dependencies_;

        void ParseAssetEntry(const YAML::Node &node);
};

}  // namespace ic

#endif