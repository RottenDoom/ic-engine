#ifndef GLTF_LOADER_H
#define GLTF_LOADER_H

#include "defines.h"

#include <fastgltf/types.hpp>
#include <fastgltf/core.hpp>
#include "model_data.h"
#include "model_loader.h"
#include "core/assets/types/asset_base.h"
#include "core/math.h"

class Model;
struct MeshPrimitive;

namespace ic
{

struct GLModel;

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
        Index texCoord2Accessor = INVALID_INDEX;
        Index jointsAccessor    = INVALID_INDEX;
        Index weightsAccessor   = INVALID_INDEX;
        Index indicesAccessor   = INVALID_INDEX;
};

class GLTFLoader : public IModelLoader
{
public:
        GLTFLoader()  = default;
        ~GLTFLoader() = default;

        // Non-copyable — owns temporary per-load state
        GLTFLoader(const GLTFLoader &)            = delete;
        GLTFLoader &operator=(const GLTFLoader &) = delete;

        bool canLoad(const char *ext) const override;
        bool load(const char *path, ModelImportData *outData) override;

private:
        bool parseAsset(fastgltf::Asset *asset, const char *basePath, ModelImportData *out);

        void loadBuffer(fastgltf::Asset *asset, const fastgltf::Buffer *src, const char *basePath, ModelImportData *out);
        void loadBufferView(const fastgltf::BufferView *src, ModelImportData *out);
        void loadAccessor(const fastgltf::Accessor *src, ModelImportData *out);

        void loadSampler(const fastgltf::Sampler *src, ModelImportData *out);
        bool loadImage(fastgltf::Asset *asset, const fastgltf::Image *src, ModelImportData *out);
        void loadTexture(const fastgltf::Texture *src, ModelImportData *out);
        void loadMaterial(const fastgltf::Material *src, ModelImportData *out);

        void loadNode(const fastgltf::Node *src, ModelImportData *out);
        void loadCamera(const fastgltf::Camera *src, ModelImportData *out);
        void loadSkin(fastgltf::Asset *asset, const fastgltf::Skin *src, ModelImportData *out);
        void loadScene(const fastgltf::Scene *src, ModelImportData *out);

        void loadMesh(const fastgltf::Mesh *src, ModelImportData *out);

        void loadAnimation(fastgltf::Asset *asset, const fastgltf::Animation *src, ModelImportData *out);

        void processMeshGeometry(ModelImportData *out);
        void extractVertices(const TempPrimitiveData *temp, MeshPrimitiveImportData *prim, ModelImportData *out);
        void extractIndices(const TempPrimitiveData *temp, MeshPrimitiveImportData *prim, ModelImportData *out);

        const uint8_t *getAccessorData(Index accessorIndex, const ModelImportData *out) const;
        void           readAccessorFloat(Index idx, std::vector<float> &outVec, const ModelImportData *out) const;
        void           readAccessorVec2(Index idx, std::vector<glm::vec2> &outVec, const ModelImportData *out) const;
        void           readAccessorVec3(Index idx, std::vector<glm::vec3> &outVec, const ModelImportData *out) const;
        void           readAccessorVec4(Index idx, std::vector<glm::vec4> &outVec, const ModelImportData *out) const;
        void           readAccessorMat4(Index idx, std::vector<glm::mat4> &outVec, const ModelImportData *out) const;
        void           readAccessorUVec4(Index idx, std::vector<glm::uvec4> &outVec, const ModelImportData *out) const;

        /** One entry per MeshPrimitive, in the same order they were pushed.
         *  Maps accessor indices recorded during loadMesh → geometry extraction. */
        std::vector<TempPrimitiveData> m_tempPrimitiveData;
};

}  // namespace ic

#endif