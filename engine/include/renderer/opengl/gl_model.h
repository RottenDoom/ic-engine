#ifndef GL_MODEL_H
#define GL_MODEL_H

#include "defines.h"
#include "core/assets/types/model.h"
#include "gl_shader.h"
#include "gl_material.h"

#include <glad/glad.h>
#include <vector>

/**
 * gl_model.h -> OpenGL GPU representation of a runtime Model.
 *
 * Ownership:
 *   GLModel        owns GLMesh[]  owns GLPrimitive[]  owns VAO/VBO/EBO handles
 *   GLModel        owns GLTexture[] indexed by Model::Image index (not texture-list index)
 *   GLModel        holds a NON-OWNING pointer to its source Model
 *                  (Model lifetime must exceed GLModel lifetime)
 *
 * No fastgltf headers. No Model* member access outside of upload().
 * All vertex layout info (attributeFlags, vertexStride) comes from
 * MeshPrimitive -> set by ic::buildModel(), not detected here.
 *
 * Attribute slot order (must match model_builder.cpp::packVertices):
 *   0  POSITION   vec3
 *   1  NORMAL     vec3
 *   2  TANGENT    vec4
 *   3  TEXCOORD0  vec2
 *   4  TEXCOORD1  vec2
 *   5  TEXCOORD2  vec2
 *   6  COLOR      vec4
 *   7  JOINTS     uvec4  (integer attrib)
 *   8  WEIGHTS    vec4
 */

namespace ic
{

// ---------------------------------------------------------------------------
// IndirectDrawCommand
// Mirrors the layout of GL_DRAW_INDIRECT_BUFFER for glDrawElementsBaseVertex.
// Stored per-primitive so we can batch into an indirect draw buffer later.
// ---------------------------------------------------------------------------

struct IndirectDrawCommand
{
        uint32_t count         = 0;
        uint32_t instanceCount = 1;
        uint32_t firstIndex    = 0;
        int32_t  baseVertex    = 0;
        uint32_t baseInstance  = 0;
};

// ---------------------------------------------------------------------------
// GLPrimitive -> GPU buffers for one draw call
// ---------------------------------------------------------------------------

struct GLPrimitive
{
        GLuint VAO       = 0;
        GLuint VBO       = 0;
        GLuint EBO       = 0;
        GLenum indexType = GL_UNSIGNED_INT;

        // Copied from MeshPrimitive -> set by buildModel(), read here.
        // Never detected by heuristic (no first-vertex inspection).
        uint32_t attributeFlags = 0;
        uint32_t vertexStride   = 0;

        IndirectDrawCommand draw;

        /**
         * Upload vertex and index data to the GPU.
         * Reads vertexData, indices, attributeFlags, vertexStride directly
         * from the already-packed MeshPrimitive.
         * Does NOT take a Model& -> it only needs the one primitive.
         */
        void setupBuffers(const MeshPrimitive &prim);

        /** Delete VAO, VBO, EBO. Safe to call multiple times (guards with 0 check). */
        void destroy();

private:
        /**
         * Configure glVertexArrayAttribFormat for each present attribute.
         * Must be called after glVertexArrayVertexBuffer is bound.
         * Attribute indices and offsets must match packVertices() order.
         */
        void setupVertexAttributes();
};

// ---------------------------------------------------------------------------
// GLMesh -> one GLPrimitive per MeshPrimitive
// ---------------------------------------------------------------------------

struct GLMesh
{
        std::vector<GLPrimitive> primitives;
};

// ---------------------------------------------------------------------------
// GLModel -> GPU mirror of a runtime Model
// ---------------------------------------------------------------------------

struct GLModel
{
        /**
         * Upload all meshes and textures from model to the GPU.
         * model must remain alive for the lifetime of this GLModel.
         * Calling upload() a second time without clearGPUMemory() first leaks GPU resources.
         */
        void upload(Model &model);

        /**
         * Delete all GPU resources. Resets to default-constructed state.
         * Safe to call if upload() was never called.
         */
        void clearGPUMemory();

        /**
         * Draw the model's default scene using the given shader.
         * Shader must already be bound.
         */
        void draw(Shader *shader, const glm::mat4 &baseTransform = glm::mat4(1.0f));

        /** Returns true if upload() has been called and clearGPUMemory() has not. */
        bool isUploaded() const { return m_model != nullptr; }

private:
        // Non-owning. Set by upload(), cleared by clearGPUMemory().
        Model *m_model = nullptr;

        // m_textures[i] corresponds to Model::images()[i] -> indexed by image index.
        // Sampler state is applied at bind time in bindMaterial().
        std::vector<GLTexture> m_textures;

        // m_meshes[i] corresponds to Model::meshes()[i].
        std::vector<GLMesh> m_meshes;

        // -----------------------------------------------------------------------
        // Internal draw helpers -> not part of the public API
        // -----------------------------------------------------------------------

        void uploadTextures();
        void uploadMeshes();

        void drawNode(Shader *shader, Index nodeIndex, const glm::mat4 &parentTransform);

        void drawMesh(Shader *shader, GLMesh *glMesh, const Mesh *mesh, const glm::mat4 &worldTransform);

        void bindMaterial(Shader *shader, const Material *mat, GLPrimitive *primitive);

        /**
         * Bind a texture slot and apply its sampler state.
         * unit    -> GL texture unit (0–15)
         * ref     -> resolved image + sampler index pair
         */
        void bindTexture(Shader *shader, const char *uniformName, int unit, const TextureRef *ref, Index samplerIdx);
};

}  // namespace ic

#endif  // GL_MODEL_H