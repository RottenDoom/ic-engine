#include "renderer/opengl/gl_model.h"
#include "core/assets/types/model.h"
#include <glad/glad.h>

namespace ic
{

void GLModel::upload(Model &model)
{
        m_model = &model;

        /** Check if the textures exist first or not */
        uploadTextures();

        uploadMeshes();
}

void GLModel::uploadTextures()
{
        const auto &images   = m_model->images();
        const auto &samplers = m_model->samplers();

        // GLModel owns one GLTexture per Image (not per Texture-list entry).
        // Materials hold resolved (image, sampler) pairs -> we just upload each image once.
        m_textures.resize(images.size());

        for (size_t i = 0; i < images.size(); ++i)
        {
                const Image img = images[i];
                if (img.pixels.empty())
                        continue;

                m_textures[i].upload(img);
        }

        // Apply sampler state per texture slot.
        // Since materials store image+sampler pairs, we apply the sampler
        // at bind time in bindMaterial() rather than here, to handle
        // the case where the same image is used with different samplers.
        // This loop pre-applies the default if no per-draw override exists.
        for (size_t i = 0; i < images.size(); ++i)
        {
                // Sampler application is deferred to bind time see bindMaterial().
                (void)samplers;
        }
}

void GLModel::uploadMeshes()
{
        const auto &meshes = m_model->meshes();
        m_meshes.resize(meshes.size());

        IC_CORE_TRACE("GLModel: uploading {} meshes", meshes.size());

        for (size_t i = 0; i < meshes.size(); ++i)
        {
                const Mesh &mesh = meshes[i];
                m_meshes[i].primitives.resize(mesh.primitives.size());

                for (size_t j = 0; j < mesh.primitives.size(); ++j)
                {
                        m_meshes[i].primitives[j].setupBuffers(mesh.primitives[j]);
                        IC_CORE_TRACE("  mesh[{}] prim[{}] EBO={}", i, j, m_meshes[i].primitives[j].EBO);
                }
        }
}

void GLModel::clearGPUMemory()
{
        for (auto &mesh : m_meshes)
                for (auto &prim : mesh.primitives)
                        prim.destroy();

        for (auto &tex : m_textures)
                tex.destroy();

        m_meshes.clear();
        m_textures.clear();
        m_model = nullptr;
}

void GLModel::draw(Shader *shader)
{
        if (!m_model)
                return;

        const Scene *scene = m_model->getDefaultScene();
        if (!scene)
                return;

        for (Index rootIdx : scene->rootNodes)
                drawNode(shader, rootIdx, glm::mat4(1.0f));
}

void GLModel::drawNode(Shader *shader, Index idx, const glm::mat4 &parentWorld)
{
        const Node *node = m_model->getNode(idx);
        if (!node)
                return;

        glm::mat4 world = parentWorld * node->localTransform;

        if (node->meshIndex != INVALID_INDEX && node->meshIndex < m_meshes.size())
        {
                const Mesh *mesh = m_model->getMesh(node->meshIndex);
                if (mesh)
                        drawMesh(shader, &m_meshes[node->meshIndex], mesh, world);
        }

        for (Index childIdx : node->children)
                drawNode(shader, childIdx, world);
}

void GLModel::drawMesh(Shader *shader, GLMesh *glMesh, const Mesh *mesh, const glm::mat4 &world)
{
        shader->setMat4("model", world);

        for (size_t i = 0; i < glMesh->primitives.size(); ++i)
        {
                if (i >= mesh->primitives.size())
                        break;

                GLPrimitive         glPrim = glMesh->primitives[i];
                const MeshPrimitive prim   = mesh->primitives[i];

                if (prim.materialIndex != INVALID_INDEX)
                {
                        const Material *mat = m_model->getMaterial(prim.materialIndex);
                        if (mat)
                                bindMaterial(shader, mat, &glPrim);
                }

                glBindVertexArray(glPrim.VAO);

                if (glPrim.draw.count > 0)
                {
                        const size_t indexSize = (glPrim.indexType == GL_UNSIGNED_SHORT) ? 2 : 4;

                        glDrawElementsBaseVertex(GL_TRIANGLES,
                                                 glPrim.draw.count,
                                                 glPrim.indexType,
                                                 reinterpret_cast<void *>(glPrim.draw.firstIndex * indexSize),
                                                 glPrim.draw.baseVertex);
                }
        }
}

void GLModel::bindMaterial(Shader *shader, const Material *mat, GLPrimitive * /*primitive*/)
{
        shader->setBool("u_useDefaultMaterial", false);

        // --- Base color ---
        shader->setVec4("u_BaseColorFactor", mat->pbr.baseColorFactor);
        if (mat->pbr.baseColorTexture.isValid())
        {
                // TextureRef->image is a direct index into Model::images → m_textures
                bindTexture(
                    shader, "u_BaseColorTexture", 0, &mat->pbr.baseColorTexture, mat->pbr.baseColorTexture.sampler);
                shader->setBool("u_HasBaseColorTexture", true);
        }
        else
        {
                shader->setBool("u_HasBaseColorTexture", false);
        }

        // --- Metallic / roughness ---
        shader->setFloat("u_MetallicFactor", mat->pbr.metallicFactor);
        shader->setFloat("u_RoughnessFactor", mat->pbr.roughnessFactor);
        if (mat->pbr.metallicRoughnessTexture.isValid())
        {
                bindTexture(shader,
                            "u_MetallicRoughnessTexture",
                            1,
                            &mat->pbr.metallicRoughnessTexture,
                            mat->pbr.metallicRoughnessTexture.sampler);
                shader->setBool("u_HasMetallicRoughnessTexture", true);
        }
        else
        {
                shader->setBool("u_HasMetallicRoughnessTexture", false);
        }

        // --- Normal ---
        if (mat->normalTexture.isValid())
        {
                bindTexture(shader, "u_NormalTexture", 2, &mat->normalTexture.ref, mat->normalTexture.ref.sampler);
                shader->setFloat("u_NormalScale", mat->normalTexture.scale);
                shader->setBool("u_HasNormalTexture", true);
        }
        else
        {
                shader->setBool("u_HasNormalTexture", false);
        }

        // --- Occlusion ---
        if (mat->occlusionTexture.isValid())
        {
                bindTexture(
                    shader, "u_OcclusionTexture", 3, &mat->occlusionTexture.ref, mat->occlusionTexture.ref.sampler);
                shader->setFloat("u_OcclusionStrength", mat->occlusionTexture.strength);
                shader->setBool("u_HasOcclusionTexture", true);
        }
        else
        {
                shader->setBool("u_HasOcclusionTexture", false);
        }

        // --- Emissive ---
        shader->setVec3("u_EmissiveFactor", mat->emissiveFactor);
        if (mat->emissiveTexture.isValid())
        {
                bindTexture(shader, "u_EmissiveTexture", 4, &mat->emissiveTexture, mat->emissiveTexture.sampler);
                shader->setBool("u_HasEmissiveTexture", true);
        }
        else
        {
                shader->setBool("u_HasEmissiveTexture", false);
        }

        // --- Alpha ---
        shader->setFloat("u_AlphaCutoff", mat->alphaCutoff);

        if (mat->alphaMode == Material::AlphaMode::Blend)
        {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        else
        {
                glDisable(GL_BLEND);
        }

        if (mat->doubleSided)
                glDisable(GL_CULL_FACE);
        else
                glEnable(GL_CULL_FACE);
}

/** Bind a texture slot and apply its sampler state. */
void GLModel::bindTexture(Shader *shader, const char *uniformName, int unit, const TextureRef *ref, Index samplerIdx)
{
        if (ref->image >= m_textures.size())
                return;

        GLuint handle = m_textures[ref->image].textureHandle;
        shader->setTexture(uniformName, unit, handle);

        // Apply sampler wrapping/filtering if one is specified
        if (samplerIdx != INVALID_INDEX)
        {
                const Sampler *sampler = m_model->getSampler(samplerIdx);
                if (sampler)
                        m_textures[ref->image].applySampler(sampler);
        }
}

// ---------------------------------------------------------------------------
// GLPrimitive
// ---------------------------------------------------------------------------

void GLPrimitive::setupBuffers(const MeshPrimitive &prim)
{
        if (prim.vertexData.empty())
        {
                IC_CORE_ERROR("GLPrimitive::setupBuffers -> empty vertexData");
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

        // Create and upload VBO
        glCreateVertexArrays(1, &VAO);
        glCreateBuffers(1, &VBO);
        glNamedBufferData(VBO, static_cast<GLsizeiptr>(prim.vertexData.size()), prim.vertexData.data(), GL_STATIC_DRAW);

        // Create and upload EBO
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

        setupVertexAttributes();

        draw.instanceCount = 1;
        draw.firstIndex    = 0;
        draw.baseVertex    = 0;
        draw.baseInstance  = 0;

        IC_CORE_TRACE("GLPrimitive: VAO={} VBO={} EBO={} indices={}", VAO, VBO, EBO, draw.count);
}

void GLPrimitive::setupVertexAttributes()
{
        // Attribute layout must match the packing order in model_builder.cpp::packVertices().
        // Order: POSITION, NORMAL, TANGENT, TEXCOORD0, TEXCOORD1, TEXCOORD2, COLOR, JOINTS, WEIGHTS

        GLuint attrib = 0;
        GLuint offset = 0;

        auto enableAttrib =
            [&](VertexAttributeFlags flag, GLint components, GLenum type, GLboolean normalized, GLuint sz)
        {
                if (!(attributeFlags & flag))
                        return;
                glEnableVertexArrayAttrib(VAO, attrib);
                if (type == GL_UNSIGNED_INT)
                        glVertexArrayAttribIFormat(VAO, attrib, components, type, offset);
                else
                        glVertexArrayAttribFormat(VAO, attrib, components, type, normalized, offset);
                glVertexArrayAttribBinding(VAO, attrib, 0);
                offset += sz;
                attrib++;
        };

        enableAttrib(ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3));
        enableAttrib(ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3));
        enableAttrib(ATTRIB_TANGENT, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4));
        enableAttrib(ATTRIB_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2));
        enableAttrib(ATTRIB_TEXCOORD1, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2));
        enableAttrib(ATTRIB_TEXCOORD2, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2));
        enableAttrib(ATTRIB_COLOR, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4));
        enableAttrib(ATTRIB_JOINTS, 4, GL_UNSIGNED_INT, GL_FALSE, sizeof(glm::uvec4));
        enableAttrib(ATTRIB_WEIGHTS, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4));
}

void GLPrimitive::destroy()
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