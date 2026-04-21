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
        bool Init(const char *assets_registry_file);

        // Save the registry file in yaml format
        bool Save(const char *assets_registry_file);
        bool IsUnsaved() { return m_registry_unsaved; }

        // Check if an asset is in the registry
        bool Contains(IC_GUID id) const;

        // Return an asset's IC_GUID based on the filepath. If filepath does not exist return INVALID_IC_GUID
        IC_GUID GetAssetId(const char *name) const;

        AssetType GetAssetType(IC_GUID id) const;

        // Return file path for a asset id
        const char *GetFilePath(IC_GUID id) const;

        // Get relative file path of a file with respect to root mount folder.
        const char *GetCachePath(IC_GUID id) const;

        const char *GetAssetName(IC_GUID id) const;
        void        SetAssetName(IC_GUID id, const string &name);

        // Register filepath into the asset registy.
        IC_GUID RegisterAsset(IC_GUID id, const char *file_path, AssetType type);
        void    RegisterDependency(IC_GUID id, IC_GUID dependency_id);

        const std::unordered_set<IC_GUID>            *GetDependencies(IC_GUID id) const;
        const std::unordered_map<IC_GUID, AssetMeta> &GetAllAssets() const { return m_assets; }

        void Unregister(IC_GUID id);

private:
        // common asset folder
        string m_asset_folder;
        bool   m_registry_unsaved = false;

        std::unordered_map<IC_GUID, AssetMeta>                   m_assets;
        std::unordered_map<string, IC_GUID>                      m_ids;
        std::unordered_map<IC_GUID, std::unordered_set<IC_GUID>> m_dependencies;

        void             ParseAssetEntry(const YAML::Node &node);
        static AssetType AssetTypeFromString(const string &s);
        static string    AssetTypeToString(AssetType type);
};

}  // namespace ic

#endif