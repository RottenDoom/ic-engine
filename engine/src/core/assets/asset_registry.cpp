#include "core/assets/asset_registry.h"
#include "core/filesystem.h"
#include "core/allocators.h"
#include "core/assets/types/asset_base.h"

#include <yaml-cpp/yaml.h>

const IC_GUID INVALID_ID = ~0u;

/** TODO:
 * 2. Hashing w.r.t types and names to get new ids for new models?
 * 3. Loading models without registry and thus editing the registry.
 * 4. More sections in the asset Registry for asset scenes models and more.
 */

namespace ic
{

AssetRegistry::AssetRegistry() {}

bool AssetRegistry::Init(const char *m_assetsregistry_file)
{
        try
        {
                const char *full = fs_getfullpath(m_assetsregistry_file);
                IC_CORE_ASSERT(full, "Invalid registry path");

                YAML::Node root = YAML::LoadFile(full);
                ic_free((void *)full);

                if (root["assets_folder"])
                        m_asset_folder = root["assets_folder"].as<string>();

                if (!root["assets"])
                {
                        IC_CORE_ERROR("Registry missing 'assets' section");
                        return false;
                }

                for (const auto &node : root["assets"])
                        ParseAssetEntry(node);

                IC_CORE_INFO("Loaded {} assets from registry", m_assets.size());
                return true;
        }
        catch (const YAML::Exception &e)
        {
                IC_CORE_ERROR("Registry parse failed: {}", e.what());
                return false;
        }
}

AssetType AssetRegistry::GetAssetType(IC_GUID id) const
{
        auto it = m_assets.find(id);
        if (it == m_assets.end())
                return AssetType::ASSET_TYPE_NONE;

        return it->second.type;
}

void AssetRegistry::ParseAssetEntry(const YAML::Node &node)
{
        if (!node["id"] || !node["filepath"])
                return;

        string  idStr = node["id"].as<string>();
        IC_GUID id    = 0;

        if (idStr.rfind("0x", 0) == 0)
                id = std::stoull(idStr, nullptr, 16);
        else
                id = std::stoull(idStr);

        AssetMeta meta;

        // ---- name ----
        if (!node["name"])
        {
                IC_CORE_WARN("TODO: Implement a fallback function that takes filename and attaches IC_GUID. Throwing "
                             "exception for now.");
        }
        string asset_name = node["name"].as<string>();
        meta.name         = asset_name;

        // ---- filepath ----
        string relPath = node["filepath"].as<string>();

        const char *full = fs_joinPath(relPath.c_str(), m_asset_folder.c_str());
        IC_CORE_ASSERT(full, "Could not join paths");

        meta.filepath = full;  // string class saves the copy
        ic_free(full);

        // ---- cache ----
        if (node["cache"])
                meta.cachePath = node["cache"].as<string>();

        // ---- type ----
        if (node["type"])
                meta.type = AssetTypeFromString(node["type"].as<string>());

        m_assets[id]     = meta;
        m_ids[meta.name] = id;

        // ---- dependencies ----
        if (node["dependencies"])
        {
                for (auto depNode : node["dependencies"])
                {
                        string  depStr = depNode.as<string>();
                        IC_GUID depId  = 0;

                        if (depStr.rfind("0x", 0) == 0)
                                depId = std::stoull(depStr, nullptr, 16);
                        else
                                depId = std::stoull(depStr);

                        m_dependencies[id].insert(depId);
                }
        }
}

bool AssetRegistry::Save(const char *m_assetsregistry_file)
{
        YAML::Emitter out;

        out << YAML::BeginMap;

        out << YAML::Key << "m_assetsfolder";
        out << YAML::Value << m_asset_folder;

        out << YAML::Key << "assets";
        out << YAML::Value << YAML::BeginSeq;

        for (const auto &[id, meta] : m_assets)
        {
                out << YAML::BeginMap;

                std::stringstream ss;
                ss << "0x" << std::hex << id;

                out << YAML::Key << "id" << YAML::Value << ss.str();
                out << YAML::Key << "filepath" << YAML::Value << meta.filepath;

                if (!meta.name.empty())
                        out << YAML::Key << "name" << YAML::Value << meta.name;

                if (!meta.cachePath.empty())
                        out << YAML::Key << "cache" << YAML::Value << meta.cachePath;

                out << YAML::Key << "type" << YAML::Value << AssetTypeToString(meta.type);

                auto depIt = m_dependencies.find(id);
                if (depIt != m_dependencies.end() && !depIt->second.empty())
                {
                        out << YAML::Key << "dependencies";
                        out << YAML::Value << YAML::BeginSeq;

                        for (IC_GUID dep : depIt->second)
                        {
                                std::stringstream depSS;
                                depSS << "0x" << std::hex << dep;
                                out << depSS.str();
                        }

                        out << YAML::EndSeq;
                }

                out << YAML::EndMap;
        }

        out << YAML::EndSeq;
        out << YAML::EndMap;

        std::ofstream fout(m_assetsregistry_file);
        fout << out.c_str();

        m_registry_unsaved = false;

        return true;
}

bool AssetRegistry::Contains(IC_GUID id) const
{
        auto it = m_assets.find(id);
        if (it == m_assets.end())
                return false;

        return true;
}

IC_GUID AssetRegistry::GetAssetId(const char *name) const
{
        auto it = m_ids.find(name);
        if (it == m_ids.end())
        {
                return INVALID_ID;
        }
        return it->second;
}

const char *AssetRegistry::GetFilePath(IC_GUID id) const
{
        auto it = m_assets.find(id);
        if (it == m_assets.end())
                return nullptr;

        return it->second.filepath.c_str();
}

const char *AssetRegistry::GetCachePath(IC_GUID id) const
{
        auto it = m_assets.find(id);
        if (it == m_assets.end())
                return nullptr;

        // Get the cache path from the registry
        const char *cachePath = fs_joinPath(it->second.cachePath.c_str(), m_asset_folder.c_str());

        // Check if the parent path exists
        const char *parent = fs_getParentPath(cachePath);

        // if the path does not exist make the directory for it.
        if (!fs_exists(parent))
        {
                fs_mkdir(parent);
        }
        ic_free(parent);

        return cachePath;
}

const char *AssetRegistry::GetAssetName(IC_GUID id) const
{
        auto it = m_assets.find(id);
        if (it == m_assets.end())
        {
                IC_CORE_WARN("AssetRegistry: Asset ID {} does not exist", id);
                return nullptr;
        }
        return it->second.name.c_str();
}

void AssetRegistry::SetAssetName(IC_GUID id, const string &name)
{
        auto it = m_assets.find(id);
        if (it == m_assets.end())
        {
                IC_CORE_WARN("AssetRegistry: Asset requested does not exist");
                return;
        }

        it->second.name    = name;
        m_registry_unsaved = true;
}

IC_GUID AssetRegistry::RegisterAsset(IC_GUID id, const char *file_path, AssetType type)
{

        AssetMeta meta;
        meta.id       = id;
        meta.filepath = file_path;
        meta.type     = type;

        m_assets[id] = meta;

        // TODO: name generator function
        m_ids[file_path]   = id;
        m_registry_unsaved = true;

        return id;
}

void AssetRegistry::RegisterDependency(IC_GUID id, IC_GUID dependency_id)
{
        m_dependencies[id].insert(dependency_id);
}

const std::unordered_set<IC_GUID> *AssetRegistry::GetDependencies(IC_GUID id) const
{
        auto it = m_dependencies.find(id);
        if (it == m_dependencies.end())
        {
                IC_CORE_INFO("No dependencies exist for asset id: {}", id);
                return nullptr;
        }
        return &it->second;
}

void AssetRegistry::Unregister(IC_GUID id)
{
        auto it = m_assets.find(id);
        if (it == m_assets.end())
                return;

        m_ids.erase(it->second.name);
        m_dependencies.erase(id);
        m_assets.erase(it);
        m_registry_unsaved = true;
}

AssetType AssetRegistry::AssetTypeFromString(const string &s)
{
        if (s == "model")
                return AssetType::ASSET_TYPE_MODEL;
        if (s == "texture")
                return AssetType::ASSET_TYPE_TEXTURE;
        if (s == "material")
                return AssetType::ASSET_TYPE_MATERIAL;
        if (s == "shader")
                return AssetType::ASSET_TYPE_SHADER;
        if (s == "skybox")
                return AssetType::ASSET_TYPE_SKYBOX;
        return AssetType::ASSET_TYPE_NONE;
}

string AssetRegistry::AssetTypeToString(AssetType type)
{
        switch (type)
        {
        case AssetType::ASSET_TYPE_MODEL:
                return "model";
        case AssetType::ASSET_TYPE_TEXTURE:
                return "texture";
        case AssetType::ASSET_TYPE_MATERIAL:
                return "material";
        case AssetType::ASSET_TYPE_SHADER:
                return "shader";
        case AssetType::ASSET_TYPE_SKYBOX:
                return "skybox";
        default:
                return "unknown";
        }
}

}  // namespace ic
