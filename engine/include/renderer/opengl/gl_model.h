#ifndef GL_MODEL_H
#define GL_MODEL_H

#include "defines.h"
#include "core/assets/types/model.h"
#include "gl_material.h"
#include "gl_sampler.h"

#include <glad/glad.h>
#include <vector>

/**
 * gl_model.h -> OpenGL GPU representation of a runtime Model.
 *
 * Ownership:
 *   GLModel  owns GLMesh[]  owns GLPrimitive[]  owns VAO/VBO/EBO handles
 *   GLModel  owns GLTexture[] indexed by Model::Image index (not texture-list index)
 *   GLModel  holds a NON-OWNING pointer to its source Model
 *            (Model lifetime must exceed GLModel lifetime)
 *
 * Drawing is NOT done by GLModel. Callers collect DrawItems via
 * collectDrawItems(), build RenderCommands, and dispatch via render passes.
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
        void SetupBuffers(const MeshPrimitive &prim);

        /** Delete VAO, VBO, EBO. Safe to call multiple times (guards with 0 check). */
        void Destroy();

private:
        /**
         * Configure glVertexArrayAttribFormat for each present attribute.
         * Must be called after glVertexArrayVertexBuffer is bound.
         * Attribute indices and offsets must match packVertices() order.
         */
        void SetupVertexAttributes();
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
        ~GLModel() { ClearGPUMemory(); }

        // Non-copyable -> owns GPU handles
        GLModel()                           = default;
        GLModel(const GLModel &)            = delete;
        GLModel &operator=(const GLModel &) = delete;

        /**
         * One primitive in the model's scene graph, ready for submission.
         * Produced by collectDrawItems(); consumed by render passes.
         */
        struct DrawItem
        {
                GLPrimitive *primitive;      // non-owning
                Index        materialIndex;  // into Model::materials(); may be INVALID_INDEX
                glm::mat4    worldTransform;
        };

        /**
         * Upload all meshes and textures from model to the GPU.
         * model must remain alive for the lifetime of this GLModel.
         * Calling upload() a second time without clearGPUMemory() first leaks GPU resources.
         */
        void Upload(Model &model);

        /**
         * Delete all GPU resources. Resets to default-constructed state.
         * Safe to call if upload() was never called.
         */
        void ClearGPUMemory();

        /**
         * Traverse the model's default scene and collect one DrawItem per primitive.
         * No GL calls — pure transform accumulation and pointer collection.
         * baseTransform is the entity's world-space transform matrix.
         */
        void CollectDrawItems(const glm::mat4 &baseTransform, std::vector<DrawItem> &out) const;

        /** Returns true if upload() has been called and clearGPUMemory() has not. */
        bool IsUploaded() const { return m_model != nullptr; }

        /** Non-owning view of uploaded textures (indexed by Model::images() index). */
        const std::vector<Texture>   &textures() const { return m_textures; }
        const std::vector<GLSampler> &samplers() const { return m_samplers; }

        /** Non-owning pointer to the source Model. Null after clearGPUMemory(). */
        Model *Get() const { return m_model; }

private:
        // Non-owning. Set by upload(), cleared by clearGPUMemory().
        Model *m_model = nullptr;

        // m_textures[i] contains gl textures and nothing else these are indexed by texture handle indices.
        std::vector<Texture>   m_textures;
        std::vector<GLSampler> m_samplers;

        // m_meshes[i] corresponds to Model::meshes()[i].
        std::vector<GLMesh> m_meshes;

        void UploadTextures();
        void UploadMeshes();

        void CollectNode(Index nodeIndex, const glm::mat4 &parentWorld, std::vector<DrawItem> &out) const;
};

}  // namespace ic

#endif  // GL_MODEL_H