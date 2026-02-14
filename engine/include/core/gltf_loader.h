#pragma once
#include "defines.h"

#include <fastgltf/types.hpp>
#include <fastgltf/core.hpp>
#include "core/assets/types/model.h"

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

struct TempPrimitiveData
{
        Index positionAccessor  = INVALID_INDEX;
        Index normalAccessor    = INVALID_INDEX;
        Index tangentAccessor   = INVALID_INDEX;
        Index colorAccessor     = INVALID_INDEX;
        Index texCoord0Accessor = INVALID_INDEX;
        Index texCoord1Accessor = INVALID_INDEX;
        Index jointsAccessor    = INVALID_INDEX;
        Index weightsAccessor   = INVALID_INDEX;
        Index indicesAccessor   = INVALID_INDEX;
};

// TODO make interface for model loader classes and make this an external class since it just loads models from physical
// directories.
class GLTFLoader
{
public:
        ~GLTFLoader();

        bool loadModel(const char *path, Model *model);

private:
        bool loadGLTF(std::filesystem::path path, Model *gltf);

        // Raw GLTF loading
        bool loadScene(Model *gltf, fastgltf::Scene &scene);
        bool loadNode(Model *gltf, fastgltf::Node &node);
        bool loadMesh(Model *gltf, fastgltf::Mesh &mesh);
        bool loadSamplers(Model *gltf, fastgltf::Sampler &sampler);
        bool loadMaterial(Model *gltf, fastgltf::Material &material);
        bool loadImage(Model *gltf, fastgltf::Asset &asset, fastgltf::Image &image);
        bool loadCamera(Model *gltf, fastgltf::Camera &camera);

        void loadTexture(Model *gltf, fastgltf::Texture &texture);
        void loadBufferView(Model *gltf, fastgltf::BufferView &bufferView);
        void loadBuffer(Model *gltf, const fastgltf::Buffer &buffer, const std::filesystem::path &basePath);
        void loadAccessor(Model *gltf, fastgltf::Accessor &accessor);

        // Geometry processing
        void processMeshGeometry(Model *gltf);
        void extractVertices(Model *gltf, const TempPrimitiveData &tempData, MeshPrimitive &primitive);
        void extractIndices(Model *gltf, const TempPrimitiveData &tempData, MeshPrimitive &primitive);

        // Accessor reading
        const uint8_t *getAccessorData(Model *gltf, Index accessorIndex);
        void readAccessorVec2(Model *gltf, Index accessorIndex, std::vector<glm::vec2> &outData);
        void readAccessorVec3(Model *gltf, Index accessorIndex, std::vector<glm::vec3> &outData);
        void readAccessorVec4(Model *gltf, Index accessorIndex, std::vector<glm::vec4> &outData);
        void readAccessorUVec4(Model *gltf, Index accessorIndex, std::vector<glm::uvec4> &outData);

        // Helpers
        Accessor::Type convertAccessorType(fastgltf::AccessorType type);

        // Temporary data for processing
        std::vector<TempPrimitiveData> m_tempPrimitiveData;
};

}  // namespace ic
