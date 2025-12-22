#pragma once
#include "defines.h"

#include "renderer/model.h"
#include "gl_shader.h"
#include "gl_texture.h"

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
        ATTRIB_TANGENT   = 1 << 2,
        ATTRIB_TEXCOORD0 = 1 << 3,
        ATTRIB_TEXCOORD1 = 1 << 4,
        ATTRIB_TEXCOORD2 = 1 << 5,
        ATTRIB_COLOR     = 1 << 6,
        ATTRIB_JOINTS    = 1 << 7,
        ATTRIB_WEIGHTS   = 1 << 8,
};

// Packed vertex structure - only contains what's needed
struct PackedVertex
{
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec4 tangent;
        glm::vec2 texcoord0;
        glm::vec2 texcoord1;
        glm::vec2 texcoord2;
        glm::vec4 color;
        glm::uvec4 joints;
        glm::vec4 weights;
};

struct GLPrimitive
{
        GLuint VAO;
        GLuint VBO;
        GLuint EBO;
        GLenum indexType;
        uint32_t attributeFlags;  // Which attributes are present
        size_t vertexStride;      // Actual stride based on present attributes
        IndirectDrawCommand draw;

        void setupBuffers(Model& model, MeshPrimitive& primitive);

private:
        void readAttribute(Model& model,
                           Index accessorIdx,
                           std::vector<uint8_t>& vertexBuffer,
                           size_t offset,
                           size_t stride,
                           size_t vertexCount);

        void readIndices(Model& model, Index accessorIdx, std::vector<uint32_t>& indexBuffer);

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
        Model* model;
        std::vector<GLMesh> meshes;
        std::vector<GLTexture> textures;

        /** TODO: Const correctness */
        void upload(Model& model);
        void uploadTextures();
        void uploadMeshes();

        /** Need shader handle here as well */
        /** TODO: Make a material system */
        void draw(Shader& shader);
        void drawNode(Shader& shader, Index nodeIndex, glm::mat4 parentTransform);
        void drawMesh(Shader& shader, GLMesh glMesh, Mesh& mesh, glm::mat4 worldTransform);

        void bindMaterial(Shader& shader, Material& mat, GLPrimitive& primitive);
};

}  // namespace ic