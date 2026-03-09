#include "core/assets/asset_registry.h"
#include "core/filesystem.h"
#include "core/allocators.h"
#include "core/assets/types/asset_base.h"

#include <yaml-cpp/yaml.h>

const GUID INVALID_ID = ~0u;

namespace ic
{

AssetRegistry::AssetRegistry() {}
static GUID UUID = 0u;

bool AssetRegistry::init(const char *assets_registry_file)
{
        try
        {
                const char *full = fs_getfullpath(assets_registry_file);
                IC_CORE_ASSERT(full, "Invalid registry path");

                YAML::Node root = YAML::LoadFile(full);
                ic_free((void *)full);

                if (root["assets_folder"])
                        assets_folder_ = root["assets_folder"].as<std::string>();

                if (!root["assets"])
                {
                        IC_CORE_ERROR("Registry missing 'assets' section");
                        return false;
                }

                for (const auto &node : root["assets"])
                        parseAssetEntry(node);

                IC_CORE_INFO("Loaded {} assets from registry", assets_.size());
                return true;
        }
        catch (const YAML::Exception &e)
        {
                IC_CORE_ERROR("Registry parse failed: {}", e.what());
                return false;
        }
}

AssetType AssetRegistry::getAssetType(GUID id) const
{
        auto it = assets_.find(id);
        if (it == assets_.end())
                return AssetType::ASSET_TYPE_NONE;

        return it->second.type;
}

void AssetRegistry::parseAssetEntry(const YAML::Node &node)
{
        if (!node["id"] || !node["filepath"])
                return;

        std::string idStr = node["id"].as<std::string>();
        GUID        id    = 0;

        if (idStr.rfind("0x", 0) == 0)
                id = std::stoull(idStr, nullptr, 16);
        else
                id = std::stoull(idStr);

        AssetMeta meta;

        // ---- filepath ----
        std::string relPath = node["filepath"].as<std::string>();

        const char *full = nullptr;
        if (!fs_joinPath(relPath.c_str(), assets_folder_.c_str(), &full))
                return;

        meta.filepath = full;
        ic_free((void *)full);

        // ---- cache ----
        if (node["cache"])
                meta.cachePath = node["cache"].as<std::string>();

        // ---- type ----
        if (node["type"])
                meta.type = assetTypeFromString(node["type"].as<std::string>());

        assets_[id]         = meta;
        ids_[meta.filepath] = id;

        // ---- dependencies ----
        if (node["dependencies"])
        {
                for (auto depNode : node["dependencies"])
                {
                        std::string depStr = depNode.as<std::string>();
                        GUID        depId  = 0;

                        if (depStr.rfind("0x", 0) == 0)
                                depId = std::stoull(depStr, nullptr, 16);
                        else
                                depId = std::stoull(depStr);

                        dependencies_[id].insert(depId);
                }
        }
}

bool AssetRegistry::save(const char *assets_registry_file)
{
        YAML::Emitter out;

        out << YAML::BeginMap;

        out << YAML::Key << "assets_folder";
        out << YAML::Value << assets_folder_;

        out << YAML::Key << "assets";
        out << YAML::Value << YAML::BeginSeq;

        for (const auto &[id, meta] : assets_)
        {
                out << YAML::BeginMap;

                std::stringstream ss;
                ss << "0x" << std::hex << id;

                out << YAML::Key << "id" << YAML::Value << ss.str();
                out << YAML::Key << "filepath" << YAML::Value << meta.filepath;

                if (!meta.cachePath.empty())
                        out << YAML::Key << "cache" << YAML::Value << meta.cachePath;

                out << YAML::Key << "type" << YAML::Value << assetTypeToString(meta.type);

                auto depIt = dependencies_.find(id);
                if (depIt != dependencies_.end() && !depIt->second.empty())
                {
                        out << YAML::Key << "dependencies";
                        out << YAML::Value << YAML::BeginSeq;

                        for (GUID dep : depIt->second)
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

        std::ofstream fout(assets_registry_file);
        fout << out.c_str();

        return true;
}
bool AssetRegistry::contains(GUID id) const
{
        auto it = assets_.find(id);
        if (it == assets_.end())
                return false;

        return fs_exists(it->second.filepath.c_str());
}

GUID AssetRegistry::getAssetId(const char *file_path) const
{
        auto it = ids_.find(file_path);
        if (it == ids_.end())
        {
                return INVALID_ID;
        }
        return it->second;
}

const char *AssetRegistry::getFilePath(GUID id) const
{
        auto it = assets_.find(id);
        if (it == assets_.end())
                return nullptr;

        return it->second.filepath.c_str();
}

const char *AssetRegistry::getCachePath(GUID id) const
{
        auto it = assets_.find(id);
        if (it == assets_.end())
                return nullptr;

        return it->second.cachePath.c_str();
}

GUID AssetRegistry::registerAsset(GUID id, const char *file_path, AssetType type)
{

        AssetMeta meta;
        meta.id       = id;
        meta.filepath = file_path;
        meta.type     = type;

        assets_[id]     = meta;
        ids_[file_path] = id;

        return id;
}

void AssetRegistry::registerDependency(GUID id, GUID dependency_id)
{
        dependencies_[id].insert(dependency_id);
}

const std::unordered_set<GUID> *AssetRegistry::getDependencies(GUID id) const
{
        auto it = dependencies_.find(id);
        if (it == dependencies_.end())
        {
                IC_CORE_INFO("No dependencies exist for asset id: {}", id);
                return nullptr;
        }
        return &it->second;
}

void AssetRegistry::unregister(GUID id)
{
        auto it = assets_.find(id);
        if (it == assets_.end())
                return;

        ids_.erase(it->second.filepath);
        dependencies_.erase(id);
        assets_.erase(it);
}

AssetType AssetRegistry::assetTypeFromString(const string &s)
{
        if (s == "model")
                return AssetType::ASSET_TYPE_MODEL;
        if (s == "texture")
                return AssetType::ASSET_TYPE_TEXTURE;
        if (s == "material")
                return AssetType::ASSET_TYPE_MATERIAL;
        if (s == "shader")
                return AssetType::ASSET_TYPE_SHADER;
        return AssetType::ASSET_TYPE_NONE;
}

string AssetRegistry::assetTypeToString(AssetType type)
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
        default:
                return "unknown";
        }
}

}  // namespace ic
