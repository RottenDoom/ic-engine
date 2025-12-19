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
        bool loadGLTF(const char* path, GLTFModel* model);
        bool loadMesh(GLTFModel* gltf, fastgltf::Mesh& mesh);
        // bool loadImage(GLTFModel* gltf, fastgltf::Image& image);
        // bool loadMaterial(GLTFModel* gltf, fastgltf::Material& material);
        // bool loadCamera(GLTFModel* gltf, fastgltf::Camera& camera);

        // void drawMesh(GLTFModel* gltf, std::vector<fastgltf::Node*>& cameraNodes, size_t nodeIndex);
        // void updateCameraNodes(GLTFModel* gltf, std::vector<fastgltf::Node*>& cameraNodes, size_t nodeIndex);

public:
        ModelType type = GLTF;
        std::string name;
};

}  // namespace ic
