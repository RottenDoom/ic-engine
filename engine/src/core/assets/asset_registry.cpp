#include "core/assets/asset_registry.h"
#include "core/filesystem.h"
#include "core/assets/types/asset_base.h"

#include <yaml-cpp/yaml.h>

const GUID INVALID_ID = ~0u;
static GUID UUID      = 0u;

namespace ic
{

AssetRegistry::AssetRegistry() {}

bool AssetRegistry::Init(const char *assets_registry_file)
{
        try
        {
                const char *full_path = fs_getfullpath(assets_registry_file);
                IC_CORE_ASSERT(full_path, "Path does not exist");
                YAML::Node root = YAML::LoadFile(full_path);  // FIX THIS SOMEHOW

                IC_CORE_INFO("Path to registry file {}", full_path);
                ic_free((void *)full_path);

                // Get assets folder
                if (root["assets_folder"])
                {
                        assets_folder_ = root["assets_folder"].as<string>();
                        IC_CORE_INFO("Assets folder: {}", assets_folder_);
                }

                // Parse all assets
                if (!root["assets"])
                {
                        IC_CORE_ERROR("No 'assets' section in registry!");
                        return false;
                }

                YAML::Node assets = root["assets"];
                for (const auto &asset_node : assets)
                {
                        ParseAssetEntry(asset_node);
                }

                IC_CORE_INFO("Loaded {} assets from registry", file_paths_.size());
                return true;
        }
        catch (const YAML::Exception &e)
        {
                IC_CORE_ERROR("Failed to parse registry: {}", e.what());
                return false;
        }
}

void AssetRegistry::ParseAssetEntry(const YAML::Node &node)
{
        // Parse ID (supports hex format)
        GUID id = 0;
        if (node["id"])
        {
                string id_str = node["id"].as<std::string>();

                // Parse hex string (0x...)
                if (id_str.substr(0, 2) == "0x")
                {
                        id = std::stoull(id_str, nullptr, 16);
                }
                else
                {
                        id = std::stoull(id_str);
                }
                IC_CORE_TRACE("Asset ID {}", id_str);
        }
        else
        {
                IC_CORE_ERROR("Asset entry missing 'id' field");
                return;
        }

        // Parse filepath
        // std::filesystem::path filepath;
        // if (node["filepath"])
        // {
        //         filepath = assets_folder_ / node["filepath"].as<std::string>();
        // }
        // else
        // {
        //         IC_CORE_ERROR("Asset {} missing 'filepath' field", id);
        //         return;
        // }

        // Register the asset
        // file_paths_[id] = filepath;
        // ids_[filepath]  = id;

        // IC_CORE_TRACE("Registered asset: {} -> {}", id, filepath.string());

        // Parse dependencies
        if (node["dependencies"])
        {
                std::unordered_set<GUID> deps;

                for (const auto &dep_node : node["dependencies"])
                {
                        std::string dep_str = dep_node.as<std::string>();
                        GUID dep_id;

                        if (dep_str.substr(0, 2) == "0x")
                        {
                                dep_id = std::stoull(dep_str, nullptr, 16);
                        }
                        else
                        {
                                dep_id = std::stoull(dep_str);
                        }

                        deps.insert(dep_id);
                }

                dependencies_[id] = deps;
                IC_CORE_TRACE("  {} dependencies", deps.size());
        }
}

bool AssetRegistry::Save(const char *assets_registry_file)
{
        try
        {
                YAML::Emitter out;

                out << YAML::BeginMap;
                out << YAML::Key << "assets_folder";
                out << YAML::Value << assets_folder_;

                out << YAML::Key << "assets";
                out << YAML::Value << YAML::BeginSeq;

                // Write each asset
                for (const auto &[id, filepath] : file_paths_)
                {
                        out << YAML::BeginMap;

                        // Write ID as hex
                        std::stringstream ss;
                        ss << "0x" << std::hex << id;
                        out << YAML::Key << "id" << YAML::Value << ss.str();

                        // Write relative filepath
                        // auto rel_path = std::filesystem::relative(filepath, assets_folder_);
                        // out << YAML::Key << "filepath" << YAML::Value << rel_path.string();

                        // Write dependencies if any
                        auto deps_it = dependencies_.find(id);
                        if (deps_it != dependencies_.end() && !deps_it->second.empty())
                        {
                                out << YAML::Key << "dependencies";
                                out << YAML::Value << YAML::BeginSeq;

                                for (GUID dep_id : deps_it->second)
                                {
                                        std::stringstream dep_ss;
                                        dep_ss << "0x" << std::hex << dep_id;
                                        out << dep_ss.str();
                                }

                                out << YAML::EndSeq;
                        }

                        out << YAML::EndMap;
                }

                out << YAML::EndSeq;
                out << YAML::EndMap;

                // Write to file
                // std::ofstream fout(registry_file);
                // fout << out.c_str();

                return true;
        }
        catch (const YAML::Exception &e)
        {
                IC_CORE_ERROR("Failed to save registry: {}", e.what());
                return false;
        }
}
bool AssetRegistry::Contains(GUID id) const
{
        // Find the element
        auto it = file_paths_.find(id);

        // Not found in registry
        if (it == file_paths_.end())
        {
                return false;
        }

        // Found in registry - check if file exists
        const auto &filepath = it->second;  // Use the iterator!

        if (fs_exists(filepath))
        {
                return true;
        }
        else
        {
                IC_CORE_WARN("The ID {} is in the registry but couldn't find the file path: {}", id, filepath);
                return false;
        }
}

GUID AssetRegistry::GetAssetId(const char *file_path) const
{
        auto it = ids_.find(file_path);
        if (it == ids_.end())
        {
                return INVALID_ID;
        }
        return it->second;
}

const char *AssetRegistry::GetFilePath(GUID id) const
{
        if (Contains(id))
        {
                return file_paths_.at(id);
        }

        IC_CORE_ERROR("Could not find file path to id: {}", id);
        return nullptr;
}

char *AssetRegistry::GetRelFilePath(GUID id) const
{
        if (Contains(id))
        {
                // fs_filename(file_paths_.at(id));
                char *path = nullptr;
                fs_joinPath(assets_folder_.c_str(), file_paths_.at(id), path);
                return path;
        }
        return nullptr;
}

GUID AssetRegistry::Register(const char *file_path)
{
        auto id = ids_.find(file_path);
        if (id != ids_.end())
        {
                if (Contains(id->second))
                {

                        IC_CORE_INFO("File path {} already in registry with id: {}", file_path, id->second);
                        return id->second;
                }
                else
                {
                        IC_CORE_INFO("File path {} exists in registry but not the mapping for id: {}. Registering now.",
                                     file_path,
                                     id->second);

                        file_paths_[id->second] = file_path;
                        return INVALID_ID;
                }
        }

        GUID NEW_ID = ++UUID; /** TODO: Chenge this to some UUID generator */
        IC_CORE_INFO("Registering file_path to id: {}");
        file_paths_[NEW_ID] = file_path;
        ids_[file_path]     = NEW_ID;
        return NEW_ID;
}

void AssetRegistry::RegisterDependency(GUID id, GUID dependency_id)
{
        auto it = dependencies_.find(id);
        if (it != dependencies_.end())
        {
                it->second.insert(dependency_id);
        }
        else
        {
                dependencies_[id] = std::unordered_set<GUID>({dependency_id});
        }
}

const std::unordered_set<GUID> *AssetRegistry::GetDependencies(GUID id) const
{
        auto it = dependencies_.find(id);
        if (it == dependencies_.end())
        {
                IC_CORE_INFO("No dependencies exist for asset id: {}", id);
                return nullptr;
        }
        return &it->second;
}

void AssetRegistry::Unregister(GUID id)
{
        auto it = file_paths_.find(id);

        if (it == file_paths_.end())
        {
                IC_CORE_WARN("Already unregistered id: {}", id);
        }

        const char *file_path = it->second;
        file_paths_.erase(id);
        ids_.erase(file_path);
        dependencies_.erase(id);
}

}  // namespace ic
