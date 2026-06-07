#include "renderer/opengl/gl_model.h"
#include "core/assets/types/model.h"
#include <glad/glad.h>

namespace ic
{

void GLModel::Upload(Model &model)
{
        m_model = &model;

        /** Check if the textures exist first or not */
        UploadTextures();

        UploadMeshes();
}

/** Load the model textures from GLmodel to the GPU for each texture
 * This just uploads the data to the gpu.
 * TODO: make a function that uploads a single image to the GLmodel data goes out of scope as soon as model loading
 * completes.
 */
void GLModel::UploadTextures()
{
        const auto &samplers = m_model->samplers();

        m_samplers.resize(samplers.size());

        // This code should be for per texture instead of here
        for (size_t i = 0; i < samplers.size(); ++i)
        {
                const Sampler s = samplers[i];
                GLSampler     gl_s;
                gl_s.Build(s);
                if (!gl_s.handle)
                {
                        IC_CORE_WARN("OpenGL sampler did not upload resorting to fallbacks");
                        continue;
                }
                m_samplers.push_back(gl_s);
        }
}

void GLModel::UploadMeshes()
{
        const auto &meshes = m_model->meshes();
        m_meshes.resize(meshes.size());

        IC_CORE_TRACE("GLModel: Uploading {} meshes", meshes.size());

        for (size_t i = 0; i < meshes.size(); ++i)
        {
                const Mesh &mesh = meshes[i];
                m_meshes[i].primitives.resize(mesh.primitives.size());

                for (size_t j = 0; j < mesh.primitives.size(); ++j)
                {
                        m_meshes[i].primitives[j].SetupBuffers(mesh.primitives[j]);
#ifndef NDEBUG
                        IC_CORE_TRACE("  mesh[{}] prim[{}] EBO={}", i, j, m_meshes[i].primitives[j].EBO);
#endif
                }
        }
}

void GLModel::ClearGPUMemory()
{
        for (auto &mesh : m_meshes)
                for (auto &prim : mesh.primitives)
                        prim.Destroy();

        m_meshes.clear();
        m_textures.clear();
        m_model = nullptr;
}

void GLModel::CollectDrawItems(const glm::mat4 &baseTransform, std::vector<DrawItem> &out) const
{
        if (!m_model)
                return;

        const Scene *scene = m_model->GetDefaultScene();
        if (!scene)
                return;

        for (Index rootIdx : scene->rootNodes)
                CollectNode(rootIdx, baseTransform, out);
}

void GLModel::CollectNode(Index nodeIdx, const glm::mat4 &parentWorld, std::vector<DrawItem> &out) const
{
        const Node *node = m_model->GetNode(nodeIdx);
        if (!node)
                return;

        glm::mat4 world = parentWorld * node->localTransform;

        if (node->meshIndex != INVALID_INDEX && node->meshIndex < m_meshes.size())
        {
                const Mesh   *mesh   = m_model->GetMesh(node->meshIndex);
                const GLMesh &glMesh = m_meshes[node->meshIndex];

                if (mesh)
                {
                        for (size_t i = 0; i < glMesh.primitives.size(); ++i)
                        {
                                if (i >= mesh->primitives.size())
                                        break;
                                out.push_back({const_cast<GLPrimitive *>(&glMesh.primitives[i]),
                                               mesh->primitives[i].materialIndex,
                                               world});
                        }
                }
        }

        for (Index childIdx : node->children)
                CollectNode(childIdx, world, out);
}

// ---------------------------------------------------------------------------
// GLPrimitive
// ---------------------------------------------------------------------------

void GLPrimitive::SetupBuffers(const MeshPrimitive &prim)
{
        if (prim.vertexData.empty())
        {
                IC_CORE_ERROR("GLPrimitive::SetupBuffers -> empty vertexData");
                return;
        }

        // All layout information is already computed by buildModel() and stored
        // on the primitive. No first-vertex heuristic needed here.
        attributeFlags = prim.attributeFlags;
        vertexStride   = prim.vertexStride;

        IC_CORE_TRACE("GLPrimitive: vertices={}, stride={}, flags=0x{:x}, totalBytes={}",
                      prim.vertexCount,
                      prim.vertexStride,
                      prim.attributeFlags,
                      prim.vertexData.size());

        // Create and Upload VBO
        glCreateVertexArrays(1, &VAO);
        glCreateBuffers(1, &VBO);
        glNamedBufferData(VBO, static_cast<GLsizeiptr>(prim.vertexData.size()), prim.vertexData.data(), GL_STATIC_DRAW);

        // Create and Upload EBO
        if (!prim.indices.empty())
        {
                glCreateBuffers(1, &EBO);
                glNamedBufferData(EBO,
                                  static_cast<GLsizeiptr>(prim.indices.size() * sizeof(uint32_t)),
                                  prim.indices.data(),
                                  GL_STATIC_DRAW);
                glVertexArrayElementBuffer(VAO, EBO);

                indexType  = GL_UNSIGNED_INT;
                draw.count = static_cast<uint32_t>(prim.indices.size());
        }
        else
        {
                EBO        = 0;
                draw.count = prim.vertexCount;
        }

        // Bind VBO to binding point 0 with the known stride
        glVertexArrayVertexBuffer(VAO, 0, VBO, 0, static_cast<GLsizei>(vertexStride));

        SetupVertexAttributes();

        draw.instanceCount = 1;
        draw.firstIndex    = 0;
        draw.baseVertex    = 0;
        draw.baseInstance  = 0;

        IC_CORE_TRACE("GLPrimitive: VAO={} VBO={} EBO={} indices={}", VAO, VBO, EBO, draw.count);
}

void GLPrimitive::SetupVertexAttributes()
{
        // Attribute layout must match the packing order in model_builder.cpp::packVertices().
        // Order: POSITION, NORMAL, TANGENT, TEXCOORD0, TEXCOORD1, TEXCOORD2, COLOR, JOINTS, WEIGHTS
        //
        // Locations are FIXED to match shader layout(location=N) declarations.
        // The offset still advances sequentially (tracks byte position in the packed vertex buffer).
        // Previously used a sequential counter which broke when optional attributes (e.g. TANGENT)
        // were absent: TEXCOORD0 would land at location 2 (TANGENT slot) instead of 3.

        GLuint offset = 0;

        auto enableAttrib = [&](GLuint               location,
                                VertexAttributeFlags flag,
                                GLint                components,
                                GLenum               type,
                                GLboolean            normalized,
                                GLuint               sz)
        {
                if (!(attributeFlags & flag))
                        return;
                glEnableVertexArrayAttrib(VAO, location);
                if (type == GL_UNSIGNED_INT)
                        glVertexArrayAttribIFormat(VAO, location, components, type, offset);
                else
                        glVertexArrayAttribFormat(VAO, location, components, type, normalized, offset);
                glVertexArrayAttribBinding(VAO, location, 0);
                offset += sz;
        };

        enableAttrib(0, ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3));
        enableAttrib(1, ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3));
        enableAttrib(2, ATTRIB_TANGENT, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4));
        enableAttrib(3, ATTRIB_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2));
        enableAttrib(4, ATTRIB_TEXCOORD1, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2));
        enableAttrib(5, ATTRIB_TEXCOORD2, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2));
        enableAttrib(6, ATTRIB_COLOR, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4));
        enableAttrib(8, ATTRIB_JOINTS, 4, GL_UNSIGNED_INT, GL_FALSE, sizeof(glm::uvec4));
        enableAttrib(9, ATTRIB_WEIGHTS, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4));
}

void GLPrimitive::Destroy()
{
        if (VAO)
        {
                glDeleteVertexArrays(1, &VAO);
                VAO = 0;
        }
        if (VBO)
        {
                glDeleteBuffers(1, &VBO);
                VBO = 0;
        }
        if (EBO)
        {
                glDeleteBuffers(1, &EBO);
                EBO = 0;
        }
}

}  // namespace ic