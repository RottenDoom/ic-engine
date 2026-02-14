#include "renderer/opengl/gl_model.h"
#include <glad/glad.h>

namespace ic
{

void GLModel::upload(Model &model)
{
        this->model = &model;

        /** Check if the textures exist first or not */
        uploadTextures();

        uploadMeshes();
}

void GLModel::uploadTextures()
{
        textures.resize(model->textures.size());

        for (size_t i = 0; i < model->textures.size(); ++i)
        {
                Texture &tex = model->textures[i];
                if (tex.image != INVALID_INDEX)
                {
                        ImageData &img = model->images[tex.image];
                        textures[i].createTexture(*model, tex, img);
                }

                // Apply sampler if present
                if (tex.sampler != INVALID_INDEX)
                {
                        Sampler &sampler = model->samplers[tex.sampler];
                        textures[i].applySampler(sampler);
                }
        }
}

void GLModel::uploadMeshes()
{
        meshes.resize(model->meshes.size());
        IC_CORE_TRACE("No. of meshes: {}", meshes.size());

        for (size_t i = 0; i < model->meshes.size(); ++i)
        {
                Mesh &mesh = model->meshes[i];

                meshes[i].primitives.resize(mesh.primitives.size());

                for (size_t j = 0; j < mesh.primitives.size(); ++j)
                {
                        meshes[i].primitives[j].setupBuffers(*model, mesh.primitives[j]);
                        IC_CORE_TRACE("Mesh primitive EBO {}", meshes[i].primitives[j].EBO);
                }
        }
}

void GLModel::draw(Shader &shader)
{
        Scene &scene = model->scenes[model->defaultScene];

        for (auto &rootNodeIdx : scene.rootNodes)
        {
                drawNode(shader, rootNodeIdx, glm::mat4(1.0f));  //** Transform needs to be checked. */
        }
}

void GLModel::drawNode(Shader &shader, Index idx, glm::mat4 parent)
{
        Node &node      = model->nodes[idx];
        glm::mat4 world = parent * node.localTransform;

        if (node.meshIndex != INVALID_INDEX)
        {
                drawMesh(shader, meshes[node.meshIndex], model->meshes[node.meshIndex], world);
        }

        for (Index childIdx : node.children)
        {
                drawNode(shader, childIdx, world);
        }
}

void GLModel::drawMesh(Shader &shader, GLMesh glMesh, Mesh &mesh, glm::mat4 world)
{

        //  Set model matrix uniform
        shader.setMat4("model", world);

        for (size_t i = 0; i < glMesh.primitives.size(); ++i)
        {
                GLPrimitive &prim       = glMesh.primitives[i];
                MeshPrimitive &meshPrim = mesh.primitives[i];

                // Bind material if present
                if (meshPrim.materialIndex != INVALID_INDEX)
                {
                        Material &mat = model->materials[meshPrim.materialIndex];
                        bindMaterial(shader, mat, prim);
                }

                glBindVertexArray(prim.VAO);

                if (prim.draw.count > 0)
                {
                        glDrawElementsBaseVertex(GL_TRIANGLES,
                                                 prim.draw.count,
                                                 prim.indexType,
                                                 (void *)(prim.draw.firstIndex *
                                                          (prim.indexType == GL_UNSIGNED_SHORT ? 2 : 4)),
                                                 prim.draw.baseVertex);
                }
        }
}

void GLModel::bindMaterial(Shader &shader, Material &mat, GLPrimitive &primitive)
{
        /** TODO: default material handling */
        shader.setInt("u_defaultMaterial", 0);
        shader.setBool("u_usedefaultMaterial", false);

        // base color
        shader.setVec4("u_BaseColorFactor", mat.pbrMaterial.baseColorFactor);
        if (mat.pbrMaterial.baseColorTexture.textureInfo.idx != INVALID_INDEX)
        {
                shader.setTexture("u_BaseColorTexture",
                                  0,
                                  textures[mat.pbrMaterial.baseColorTexture.textureInfo.idx].textureHandle);
                shader.setBool("u_HasBaseColorTexture", true);
        }
        else
        {
                shader.setBool("u_HasBaseColorTexture", false);
        }

        shader.setFloat("u_MetallicFactor", mat.pbrMaterial.metallicFactor);
        shader.setFloat("u_RoughnessFactor", mat.pbrMaterial.roughnessFactor);
        if (mat.pbrMaterial.metallicRoughnessTexture.textureInfo.idx != INVALID_INDEX)
        {
                shader.setTexture("u_MetallicRoughnessTexture",
                                  1,
                                  textures[mat.pbrMaterial.metallicRoughnessTexture.textureInfo.idx].textureHandle);
                shader.setBool("u_HasMetallicRoughnessTexture", true);
        }
        else
        {
                shader.setBool("u_HasMetallicRoughnessTexture", false);
        }

        // Normal
        if (mat.normalTexture.textureInfo.idx != INVALID_INDEX)
        {
                shader.setTexture("u_NormalTexture", 2, textures[mat.normalTexture.textureInfo.idx].textureHandle);
                shader.setFloat("u_NormalScale", mat.normalTexture.scale);
                shader.setBool("u_HasNormalTexture", true);
        }
        else
        {
                shader.setBool("u_HasNormalTexture", false);
        }

        // Occlusion
        if (mat.occlusionTexture.textureInfo.idx != INVALID_INDEX)
        {
                shader.setTexture("u_OcclusionTexture",
                                  3,
                                  textures[mat.occlusionTexture.textureInfo.idx].textureHandle);
                shader.setFloat("u_OcclusionStrength", mat.occlusionTexture.strength);
                shader.setBool("u_HasOcclusionTexture", true);
        }
        else
        {
                shader.setBool("u_HasOcclusionTexture", false);
        }

        // Emissive
        shader.setVec3("u_EmissiveFactor", mat.emissiveFactor);

        if (mat.emissiveTexture.textureInfo.idx != INVALID_INDEX)
        {
                shader.setTexture("u_EmissiveTexture", 4, textures[mat.emissiveTexture.textureInfo.idx].textureHandle);
                shader.setBool("u_HasEmissiveTexture", true);
        }
        else
        {
                shader.setBool("u_HasEmissiveTexture", false);
        }

        // Alpha
        shader.setFloat("u_AlphaCutoff", mat.alphaCutoff);

        // Set render state based on material
        if (mat.alphaMode == Material::AlphaMode::BLEND)
        {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        else
        {
                glDisable(GL_BLEND);
        }

        if (mat.doubleSided)
        {
                glDisable(GL_CULL_FACE);
        }
        else
        {
                glEnable(GL_CULL_FACE);
        }
}

void GLPrimitive::setupBuffers(Model &model, MeshPrimitive &primitive)
{
        // Check if we have vertices
        if (primitive.vertices.empty())
        {
                IC_CORE_ERROR("Primitive has no vertices!");
                return;
        }

        size_t vertexCount = primitive.vertices.size();

        // Determine which attributes are present by checking the first vertex
        attributeFlags = 0;
        IC_CORE_TRACE("Active vertex attributes:");

        // Position is always present (we checked vertices.empty() above)
        attributeFlags |= ATTRIB_POSITION;
        IC_CORE_TRACE("  - POSITION");

        // Check first vertex to see what attributes have non-zero/valid data
        const Vertex &firstVert = primitive.vertices[0];

        // Check if normals are present (non-zero normal indicates it's present)
        if (glm::length(firstVert.normal) > 0.0f)
        {
                IC_CORE_TRACE("  - NORMAL");
                attributeFlags |= ATTRIB_NORMAL;
        }

        // Check UVs
        if (firstVert.uv0 != glm::vec2(0.0f))
        {
                IC_CORE_TRACE("  - TEXCOORD0");
                attributeFlags |= ATTRIB_TEXCOORD0;
        }

        if (firstVert.uv1 != glm::vec2(0.0f))
        {
                IC_CORE_TRACE("  - TEXCOORD1");
                attributeFlags |= ATTRIB_TEXCOORD1;
        }

        if (firstVert.uv2 != glm::vec2(0.0f))
        {
                IC_CORE_TRACE("  - TEXCOORD2");
                attributeFlags |= ATTRIB_TEXCOORD2;
        }

        // Check color
        if (firstVert.color != glm::vec4(0.0f))
        {
                IC_CORE_TRACE("  - COLOR");
                attributeFlags |= ATTRIB_COLOR;
        }

        // Check tangent
        if (glm::length(glm::vec3(firstVert.tangent)) > 0.0f)
        {
                IC_CORE_TRACE("  - TANGENT");
                attributeFlags |= ATTRIB_TANGENT;
        }

        // Check skinning data
        if (firstVert.joint0 != glm::uvec4(0))
        {
                IC_CORE_TRACE("  - JOINTS");
                attributeFlags |= ATTRIB_JOINTS;
        }

        if (glm::length(firstVert.weight0) > 0.0f)
        {
                IC_CORE_TRACE("  - WEIGHTS");
                attributeFlags |= ATTRIB_WEIGHTS;
        }

        // Calculate vertex stride based on present attributes
        vertexStride = calculateStride(attributeFlags);

        // Allocate interleaved vertex buffer
        std::vector<uint8_t> vertexBuffer(vertexCount * vertexStride, 0);

        // Pack vertices into interleaved format
        for (size_t i = 0; i < vertexCount; ++i)
        {
                const Vertex &vert = primitive.vertices[i];
                uint8_t *dst       = vertexBuffer.data() + i * vertexStride;
                size_t offset      = 0;

                // Write each attribute that's present
                if (attributeFlags & ATTRIB_POSITION)
                {
                        memcpy(dst + offset, &vert.pos, sizeof(glm::vec3));
                        offset += sizeof(glm::vec3);
                }

                if (attributeFlags & ATTRIB_NORMAL)
                {
                        memcpy(dst + offset, &vert.normal, sizeof(glm::vec3));
                        offset += sizeof(glm::vec3);
                }

                if (attributeFlags & ATTRIB_TANGENT)
                {
                        memcpy(dst + offset, &vert.tangent, sizeof(glm::vec4));
                        offset += sizeof(glm::vec4);
                }

                if (attributeFlags & ATTRIB_TEXCOORD0)
                {
                        memcpy(dst + offset, &vert.uv0, sizeof(glm::vec2));
                        offset += sizeof(glm::vec2);
                }

                if (attributeFlags & ATTRIB_TEXCOORD1)
                {
                        memcpy(dst + offset, &vert.uv1, sizeof(glm::vec2));
                        offset += sizeof(glm::vec2);
                }

                if (attributeFlags & ATTRIB_TEXCOORD2)
                {
                        memcpy(dst + offset, &vert.uv2, sizeof(glm::vec2));
                        offset += sizeof(glm::vec2);
                }

                if (attributeFlags & ATTRIB_COLOR)
                {
                        memcpy(dst + offset, &vert.color, sizeof(glm::vec4));
                        offset += sizeof(glm::vec4);
                }

                if (attributeFlags & ATTRIB_JOINTS)
                {
                        memcpy(dst + offset, &vert.joint0, sizeof(glm::uvec4));
                        offset += sizeof(glm::uvec4);
                }

                if (attributeFlags & ATTRIB_WEIGHTS)
                {
                        memcpy(dst + offset, &vert.weight0, sizeof(glm::vec4));
                        offset += sizeof(glm::vec4);
                }
        }

        // Create OpenGL buffers
        glCreateVertexArrays(1, &VAO);
        glCreateBuffers(1, &VBO);

        // Upload vertex data
        glNamedBufferData(VBO, vertexBuffer.size(), vertexBuffer.data(), GL_STATIC_DRAW);

        IC_CORE_TRACE("Uploading VBO: vertices={}, stride={}, totalBytes={}",
                      vertexCount,
                      vertexStride,
                      vertexBuffer.size());

        // Upload index data if present
        if (!primitive.indices.empty())
        {
                glCreateBuffers(1, &EBO);
                glNamedBufferData(EBO,
                                  primitive.indices.size() * sizeof(uint32_t),
                                  primitive.indices.data(),
                                  GL_STATIC_DRAW);
                glVertexArrayElementBuffer(VAO, EBO);

                IC_CORE_TRACE("Uploading EBO: indices={}, indexType=GL_UNSIGNED_INT, totalBytes={}",
                              primitive.indices.size(),
                              primitive.indices.size() * sizeof(uint32_t));

                indexType  = GL_UNSIGNED_INT;
                draw.count = static_cast<uint32_t>(primitive.indices.size());
        }
        else
        {
                EBO        = 0;
                draw.count = static_cast<uint32_t>(vertexCount);
        }

        // Setup vertex attributes
        glVertexArrayVertexBuffer(VAO, 0, VBO, 0, vertexStride);
        setupVertexAttributes();

        // Setup draw command
        draw.instanceCount = 1;
        draw.firstIndex    = 0;
        draw.baseVertex    = 0;
        draw.baseInstance  = 0;

        IC_CORE_TRACE("==== Primitive Layout ====");
        IC_CORE_TRACE("VAO: {}", VAO);
        IC_CORE_TRACE("VBO: {}", VBO);
        IC_CORE_TRACE("EBO: {}", EBO);
        IC_CORE_TRACE("Vertex stride: {}", vertexStride);
        IC_CORE_TRACE("Vertex count: {}", vertexCount);
        IC_CORE_TRACE("Index count: {}", draw.count);
        IC_CORE_TRACE("==========================");
}

void GLPrimitive::setupVertexAttributes()
{
        GLuint attribIndex = 0;

        if (attributeFlags & ATTRIB_POSITION)
        {
                glEnableVertexArrayAttrib(VAO, attribIndex);
                glVertexArrayAttribFormat(
                    VAO, attribIndex, 3, GL_FLOAT, GL_FALSE, getAttributeOffset(attributeFlags, ATTRIB_POSITION));
                glVertexArrayAttribBinding(VAO, attribIndex, 0);
                attribIndex++;
        }

        if (attributeFlags & ATTRIB_NORMAL)
        {
                glEnableVertexArrayAttrib(VAO, attribIndex);
                glVertexArrayAttribFormat(
                    VAO, attribIndex, 3, GL_FLOAT, GL_FALSE, getAttributeOffset(attributeFlags, ATTRIB_NORMAL));
                glVertexArrayAttribBinding(VAO, attribIndex, 0);
                attribIndex++;
        }

        if (attributeFlags & ATTRIB_TEXCOORD0)
        {
                glEnableVertexArrayAttrib(VAO, attribIndex);
                glVertexArrayAttribFormat(
                    VAO, attribIndex, 2, GL_FLOAT, GL_FALSE, getAttributeOffset(attributeFlags, ATTRIB_TEXCOORD0));
                glVertexArrayAttribBinding(VAO, attribIndex, 0);
                attribIndex++;
        }

        if (attributeFlags & ATTRIB_TEXCOORD1)
        {
                glEnableVertexArrayAttrib(VAO, attribIndex);
                glVertexArrayAttribFormat(
                    VAO, attribIndex, 2, GL_FLOAT, GL_FALSE, getAttributeOffset(attributeFlags, ATTRIB_TEXCOORD1));
                glVertexArrayAttribBinding(VAO, attribIndex, 0);
                attribIndex++;
        }

        if (attributeFlags & ATTRIB_TEXCOORD2)
        {
                glEnableVertexArrayAttrib(VAO, attribIndex);
                glVertexArrayAttribFormat(
                    VAO, attribIndex, 2, GL_FLOAT, GL_FALSE, getAttributeOffset(attributeFlags, ATTRIB_TEXCOORD2));
                glVertexArrayAttribBinding(VAO, attribIndex, 0);
                attribIndex++;
        }

        if (attributeFlags & ATTRIB_COLOR)
        {
                glEnableVertexArrayAttrib(VAO, attribIndex);
                glVertexArrayAttribFormat(
                    VAO, attribIndex, 4, GL_FLOAT, GL_FALSE, getAttributeOffset(attributeFlags, ATTRIB_COLOR));
                glVertexArrayAttribBinding(VAO, attribIndex, 0);
                attribIndex++;
        }

        if (attributeFlags & ATTRIB_TANGENT)
        {
                glEnableVertexArrayAttrib(VAO, attribIndex);
                glVertexArrayAttribFormat(
                    VAO, attribIndex, 4, GL_FLOAT, GL_FALSE, getAttributeOffset(attributeFlags, ATTRIB_TANGENT));
                glVertexArrayAttribBinding(VAO, attribIndex, 0);
                attribIndex++;
        }

        if (attributeFlags & ATTRIB_JOINTS)
        {
                glEnableVertexArrayAttrib(VAO, attribIndex);
                glVertexArrayAttribIFormat(
                    VAO, attribIndex, 4, GL_UNSIGNED_INT, getAttributeOffset(attributeFlags, ATTRIB_JOINTS));
                glVertexArrayAttribBinding(VAO, attribIndex, 0);
                attribIndex++;
        }

        if (attributeFlags & ATTRIB_WEIGHTS)
        {
                glEnableVertexArrayAttrib(VAO, attribIndex);
                glVertexArrayAttribFormat(
                    VAO, attribIndex, 4, GL_FLOAT, GL_FALSE, getAttributeOffset(attributeFlags, ATTRIB_WEIGHTS));
                glVertexArrayAttribBinding(VAO, attribIndex, 0);
                attribIndex++;
        }
}

size_t GLPrimitive::calculateStride(uint32_t flags)
{
        size_t stride = 0;

        if (flags & ATTRIB_POSITION)
                stride += sizeof(glm::vec3);
        if (flags & ATTRIB_NORMAL)
                stride += sizeof(glm::vec3);
        if (flags & ATTRIB_TEXCOORD0)
                stride += sizeof(glm::vec2);
        if (flags & ATTRIB_TEXCOORD1)
                stride += sizeof(glm::vec2);
        if (flags & ATTRIB_TEXCOORD2)
                stride += sizeof(glm::vec2);
        if (flags & ATTRIB_COLOR)
                stride += sizeof(glm::vec4);
        if (flags & ATTRIB_TANGENT)
                stride += sizeof(glm::vec4);
        if (flags & ATTRIB_JOINTS)
                stride += sizeof(glm::uvec4);
        if (flags & ATTRIB_WEIGHTS)
                stride += sizeof(glm::vec4);

        return stride;
}

size_t GLPrimitive::getAttributeOffset(uint32_t flags, VertexAttributeFlags attrib)
{
        size_t offset = 0;

        // Calculate offset by summing sizes of attributes that come before
        if (attrib == ATTRIB_POSITION)
                return 0;
        if (flags & ATTRIB_POSITION)
                offset += sizeof(glm::vec3);

        if (attrib == ATTRIB_NORMAL)
                return offset;
        if (flags & ATTRIB_NORMAL)
                offset += sizeof(glm::vec3);

        if (attrib == ATTRIB_TANGENT)
                return offset;
        if (flags & ATTRIB_TANGENT)
                offset += sizeof(glm::vec4);

        if (attrib == ATTRIB_TEXCOORD0)
                return offset;
        if (flags & ATTRIB_TEXCOORD0)
                offset += sizeof(glm::vec2);

        if (attrib == ATTRIB_TEXCOORD1)
                return offset;
        if (flags & ATTRIB_TEXCOORD1)
                offset += sizeof(glm::vec2);

        if (attrib == ATTRIB_COLOR)
                return offset;
        if (flags & ATTRIB_COLOR)
                offset += sizeof(glm::vec4);

        if (attrib == ATTRIB_JOINTS)
                return offset;
        if (flags & ATTRIB_JOINTS)
                offset += sizeof(glm::uvec4);

        if (attrib == ATTRIB_WEIGHTS)
                return offset;

        return offset;
}

}  // namespace ic