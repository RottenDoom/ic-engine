#pragma once
#include "defines.h"

#include "renderer/model.h"
#include "renderer/asset_manager.h"

#include <fastgltf/types.hpp>
#include <fastgltf/core.hpp>

namespace ic
{

/** TODO: Put these structs into there respective locations */
struct GLModel : public Model
{
        fastgltf::Asset asset;
};

struct VKModel : public Model
{
        fastgltf::Asset asset;
};

#if IC_ENGINE_USE_OPENGL
using GLTFModel = GLModel;
#elif IC_ENGINE_USE_VULKAN
using GLTFModel = VKModel;
#endif

class GLTFLoader : IModelLoader
{
public:
        GLTFLoader() = default;
        ~GLTFLoader();

        bool loadModel(const char* path, Model* model) override;
        // void unloadModel();

private:
        /** TODO: These functions need to be included in a asset submodule */
        bool loadGLTF(std::filesystem::path path, GLTFModel* model);
        bool loadScene(GLTFModel* gltf, fastgltf::Scene& scene);
        bool loadNode(GLTFModel* gltf, fastgltf::Node& node);
        bool loadMesh(GLTFModel* gltf, fastgltf::Mesh& mesh);
        bool loadSamplers(GLTFModel* gltf, fastgltf::Sampler& sampler);
        bool loadMaterial(GLTFModel* gltf, fastgltf::Material& material);

        /** Loads using stb_image for now will later switch to KTX2 for GPU uploads */
        bool loadImage(GLTFModel* gltf, fastgltf::Image& image);
        bool loadTexture(GLTFModel* gltf, fastgltf::Texture& texture);
        bool loadCamera(GLTFModel* gltf, fastgltf::Camera& camera);

        // void drawMesh(GLTFModel* gltf, std::vector<fastgltf::Node*>& cameraNodes, size_t nodeIndex);
        // void updateCameraNodes(GLTFModel* gltf, std::vector<fastgltf::Node*>& cameraNodes, size_t nodeIndex);

        // void processNode(GLTFModel* gltf);

public:
        ModelType type = GLTF;
        std::string name;
};

}  // namespace ic
