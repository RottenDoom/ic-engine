#pragma once
#include "defines.h"

#include "renderer/model.h"
#include "gl_shader.h"
#include "gl_material.h"

#include <glad/glad.h>

#include <fastgltf/types.hpp>
#include <fastgltf/core.hpp>

namespace ic
{

struct IndirectDrawCommand
{
        uint32_t count;
        uint32_t instanceCount;
        uint32_t firstIndex;
        int32_t baseVertex;
        uint32_t baseInstance;
};

// Vertex attribute flags to track what's present
enum VertexAttributeFlags : uint32_t
{
        ATTRIB_POSITION  = 1 << 0,
        ATTRIB_NORMAL    = 1 << 1,
        ATTRIB_TEXCOORD0 = 1 << 2,
        ATTRIB_TEXCOORD1 = 1 << 3,
        ATTRIB_TEXCOORD2 = 1 << 4,
        ATTRIB_COLOR     = 1 << 5,
        ATTRIB_TANGENT   = 1 << 6,
        ATTRIB_JOINTS    = 1 << 7,
        ATTRIB_WEIGHTS   = 1 << 8,
};

struct GLPrimitive
{
        GLuint VAO       = 0;
        GLuint VBO       = 0;
        GLuint EBO       = 0;
        GLenum indexType = GL_UNSIGNED_BYTE;
        uint32_t attributeFlags;  // Which attributes are present
        size_t vertexStride = 0;  // Actual stride based on present attributes
        IndirectDrawCommand draw;

        void setupBuffers(Model &model, MeshPrimitive &primitive);

private:
        void readAttribute(Model &model,
                           Index accessorIdx,
                           std::vector<uint8_t> &vertexBuffer,
                           size_t offset,
                           size_t stride,
                           size_t vertexCount);

        void readIndices(Model &model, Index accessorIdx, std::vector<uint32_t> &indexBuffer);

        void setupVertexAttributes();

        size_t calculateStride(uint32_t flags);
        size_t getAttributeOffset(uint32_t flags, VertexAttributeFlags attrib);
};

struct GLMesh
{
        std::vector<GLPrimitive> primitives;
};

struct GLModel
{
        Model *model;
        std::vector<GLMesh> meshes;
        std::vector<GLTexture> textures;

        void upload(Model &model);
        void uploadTextures();
        void uploadMeshes();

        void draw(Shader &shader);
        void drawNode(Shader &shader, Index nodeIndex, glm::mat4 parentTransform);
        void drawMesh(Shader &shader, GLMesh glMesh, Mesh &mesh, glm::mat4 worldTransform);

        void bindMaterial(Shader &shader, Material &mat, GLPrimitive &primitive);
};

}  // namespace ic