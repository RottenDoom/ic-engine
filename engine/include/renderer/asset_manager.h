#pragma once
#include "../defines.h"

/** TODO: TONE DOWN the loadModel function call everywhere */

namespace ic
{
struct Model;

class IModelLoader
{
public:
        typedef enum
        {
                NONE,
                GLTF,
                OBJ,
                FBX
        } ModelType;
        ModelType type                                         = NONE;
        virtual bool loadModel(const char* path, Model* model) = 0;
        virtual ~IModelLoader() {}
};

namespace asset
{
// TODO: add more defines

using ModelHandle                          = uint32_t;
constexpr ModelHandle INVALID_MODEL_HANDLE = ~1u;  // ~0u is used for INDEX

IC_API ModelHandle LoadModel(const char* path);
IC_API void DrawModel(ModelHandle handle);
}  // namespace asset

class ShaderManager
{
public:
        ShaderManager() = default;
};

class ModelManager
{
public:
        ModelManager();

        asset::ModelHandle loadModel(const char* path);
        void draw(asset::ModelHandle handle);

private:
        std::unordered_map<const char*, ic::IModelLoader*> m_loaders;
        std::vector<asset::ModelHandle> m_models;

        /** TODO: UUID */
        asset::ModelHandle getNewHandle() { return ++m_HandleCount; };

        void registerLoader(ic::IModelLoader* modelLoader);
        size_t m_HandleCount;
};

class AssetManager
{
public:
        ModelManager modelManager{};
        ShaderManager shaderManager{};

        AssetManager();
        ~AssetManager();

        static AssetManager* Get();
        static void destroyInstance();

private:
        static AssetManager* s_instance;
};

}  // namespace ic