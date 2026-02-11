#pragma once
#include "../../defines.h"

#include "renderer/model.h"
#include "core/assets/asset_manager.h"

#include <fastgltf/types.hpp>
#include <fastgltf/core.hpp>

namespace ic
{

struct GLModel;
struct VKModel : public Model
{
        fastgltf::Asset asset;
};

// GOOD architecture says that each model loader should not know about any other model loaders
typedef enum ModelType
{
        LOADER_NONE = 0,
        LOADER_GLTF,
        LOADER_OBJ,
        LOADER_FBX
} ModelType;

// TODO make interface for model loader classes and make this an external class since it just loads models from physical
// directories.
class GLTFLoader
{
public:
        GLTFLoader() = default;
        ~GLTFLoader();

        bool loadModel(const char *path, Model *model);
        /** TODO:write a unload model function */
        // void unloadModel();

private:
        bool loadGLTF(std::filesystem::path path, Model *model);
        bool loadScene(Model *gltf, fastgltf::Scene &scene);
        bool loadNode(Model *gltf, fastgltf::Node &node);
        bool loadMesh(Model *gltf, fastgltf::Mesh &mesh);
        bool loadSamplers(Model *gltf, fastgltf::Sampler &sampler);
        bool loadMaterial(Model *gltf, fastgltf::Material &material);
        Accessor::Type convertAccessorType(fastgltf::AccessorType type);
        void loadAccessor(Model *gltf, fastgltf::Accessor &accessor);
        void loadBufferView(Model *gltf, fastgltf::BufferView &bufferView);
        void loadBuffer(Model *gltf, const fastgltf::Buffer &buffer, const std::filesystem::path &basePath);

        /** Loads using stb_image for now will later switch to KTX2 for GPU uploads */
        bool loadImage(Model *gltf, fastgltf::Asset &asset, fastgltf::Image &image);
        void loadTexture(Model *gltf, fastgltf::Texture &texture);
        bool loadCamera(Model *gltf, fastgltf::Camera &camera);

public:
        ModelType type = ModelType::LOADER_GLTF;
        std::string name;
};

}  // namespace ic
