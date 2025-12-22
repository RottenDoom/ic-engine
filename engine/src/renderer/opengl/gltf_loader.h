#pragma once
#include "defines.h"

#include "renderer/model.h"
#include "renderer/asset_manager.h"

#include <glad/glad.h>
#include <fastgltf/types.hpp>
#include <fastgltf/core.hpp>

namespace ic
{

struct GLModel;
struct VKModel : public Model
{
        fastgltf::Asset asset;
};

/** See into this later */
// #if IC_ENGINE_USE_OPENGL
// using GLTFModel = GLModel;
// #elif IC_ENGINE_USE_VULKAN
// using GLTFModel = VKModel;
// #endif

class GLTFLoader : IModelLoader
{
public:
        GLTFLoader() = default;
        ~GLTFLoader();

        bool loadModel(const char* path, Model* model) override;
        // void unloadModel();

private:
        /** TODO: These functions need to be included in a asset submodule */
        bool loadGLTF(std::filesystem::path path, Model* model);
        bool loadScene(Model* gltf, fastgltf::Scene& scene);
        bool loadNode(Model* gltf, fastgltf::Node& node);
        bool loadMesh(Model* gltf, fastgltf::Mesh& mesh);
        bool loadSamplers(Model* gltf, fastgltf::Sampler& sampler);
        bool loadMaterial(Model* gltf, fastgltf::Material& material);
        Accessor::Type convertAccessorType(fastgltf::AccessorType type);
        void loadAccessor(Model* gltf, fastgltf::Accessor& accessor);
        void loadBufferView(Model* gltf, fastgltf::BufferView& bufferView);
        void loadBuffer(Model* gltf, const fastgltf::Buffer& buffer, const std::filesystem::path& basePath);

        /** Loads using stb_image for now will later switch to KTX2 for GPU uploads */
        bool loadImage(Model* gltf, fastgltf::Asset& asset, fastgltf::Image& image);
        void loadTexture(Model* gltf, fastgltf::Texture& texture);
        bool loadCamera(Model* gltf, fastgltf::Camera& camera);

public:
        ModelType type = GLTF;
        std::string name;
};

// Helper to get component count from accessor type
inline size_t getAccessorComponentCount(Accessor::Type type)
{
        switch (type)
        {
        case Accessor::Type::SCALAR:
                return 1;
        case Accessor::Type::VEC2:
                return 2;
        case Accessor::Type::VEC3:
                return 3;
        case Accessor::Type::VEC4:
                return 4;
        case Accessor::Type::MAT4:
                return 16;
        default:
                return 0;
        }
}

// Helper to get component size from GL type
inline size_t getComponentSize(GLenum componentType)
{
        switch (componentType)
        {
        case GL_BYTE:
        case GL_UNSIGNED_BYTE:
                return 1;
        case GL_SHORT:
        case GL_UNSIGNED_SHORT:
                return 2;
        case GL_UNSIGNED_INT:
        case GL_FLOAT:
                return 4;
        default:
                return 0;
        }
}

// Helper to extract values from AccessorBoundsArray
inline std::vector<double> extractBoundsArray(const fastgltf::AccessorBoundsArray& bounds)
{
        std::vector<double> result(bounds.size());

        if (bounds.type() == fastgltf::AccessorBoundsArray::BoundsType::float64)
        {
                const double* data = bounds.data<double>();
                std::copy(data, data + bounds.size(), result.begin());
        }
        else if (bounds.type() == fastgltf::AccessorBoundsArray::BoundsType::int64)
        {
                const int64_t* data = bounds.data<int64_t>();
                std::transform(data,
                               data + bounds.size(),
                               result.begin(),
                               [](int64_t val) { return static_cast<double>(val); });
        }

        return result;
}

// Helper to get bounding box from accessor min/max
inline glm::vec3 getBoundingBoxMin(const Accessor& accessor)
{
        if (accessor.min.size() >= 3)
        {
                return glm::vec3(static_cast<float>(accessor.min[0]),
                                 static_cast<float>(accessor.min[1]),
                                 static_cast<float>(accessor.min[2]));
        }
        return glm::vec3(0.0f);
}

inline glm::vec3 getBoundingBoxMax(const Accessor& accessor)
{
        if (accessor.max.size() >= 3)
        {
                return glm::vec3(static_cast<float>(accessor.max[0]),
                                 static_cast<float>(accessor.max[1]),
                                 static_cast<float>(accessor.max[2]));
        }
        return glm::vec3(0.0f);
}

}  // namespace ic
