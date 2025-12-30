#include "renderer/asset_manager.h"
#include <filesystem>

namespace ic
{
AssetManager* AssetManager::s_instance = nullptr;

AssetManager::AssetManager() {}

AssetManager::~AssetManager() {}

AssetManager* AssetManager::Get()
{
        if (s_instance == nullptr)
        {
                s_instance = new AssetManager();
        }
        return s_instance;
}

void AssetManager::destroyInstance()
{
        if (s_instance != nullptr)
        {
                delete s_instance;
                s_instance = nullptr;
        }
}

ModelManager::ModelManager() {}

ModelHandle ModelManager::loadModel(const char* path)
{
        Model* model;
        m_loaders["GLTF"]->loadModel(path,
                                     model); /** TODO: FIX check the type of the loader first before handling this  */
        ModelHandle handle = getNewHandle();

        if (handle == INVALID_MODEL_HANDLE)
        {
                IC_CORE_WARN("Invalid Model Handle Generated"); /** TODO: better handling of Model handles */
        }
        m_models.push_back(handle);
        return handle;
}

void ModelManager::draw(ModelHandle handle) {}

void ModelManager::registerLoader(ic::IModelLoader* modelLoader)
{
        switch (modelLoader->type)
        {
        case IModelLoader::GLTF:
                m_loaders["GLTF"] = modelLoader;
                IC_CORE_INFO("Registered GLTF model Loader");
                break;
        case IModelLoader::OBJ:
                IC_CORE_WARN("OBJ extension not supported yet. Change model loader!");
                break;
        case IModelLoader::NONE:
                IC_CORE_ERROR("NONE type loader registration request. Change loader type.");
                break;
        default:
                IC_CORE_ERROR("Unknown Loader Type");
                break;
        }
}

}  // namespace ic

/** TODO: write out these functions */
ModelHandle loadModelFromFile(const char* path, float* translate, float* scale, float* rotation)
{
        return -1;
}
void unLoadModel(ModelHandle modelId) {}

void drawModel(ModelHandle modelId) {}
