#include "gl_model.h"

namespace ic
{

void GLModel::upload(Model& model)
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
                Texture& tex = model->textures[i];
                if (tex.image != INVALID_INDEX)
                {
                        ImageData& img = model->images[tex.image];
                        textures[i].createTexture(*model, tex, img);
                }

                // Apply sampler if present
                if (tex.sampler != INVALID_INDEX)
                {
                        Sampler& sampler = model->samplers[tex.sampler];
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
                Mesh& mesh = model->meshes[i];

                meshes[i].primitives.resize(mesh.meshPrimitives.size());

                for (size_t j = 0; j < mesh.meshPrimitives.size(); ++j)
                {
                        meshes[i].primitives[j].setupBuffers(*model, mesh.meshPrimitives[j]);
                        IC_CORE_TRACE("Mesh primitive EBO {}", meshes[i].primitives[j].EBO);
                }
        }
}

void GLModel::draw(Shader& shader)
{
        ic::Scene& scene = model->scenes[model->defaultScene];

        for (auto& rootNodeIdx : scene.rootNodes)
        {
                drawNode(shader, rootNodeIdx, glm::mat4(1.0f));  //** Transform needs to be checked. */
        }
}

void GLModel::drawNode(Shader& shader, Index idx, glm::mat4 parent)
{
        Node& node      = model->nodes[idx];
        glm::mat4 world = parent * node.localTransform;

        if (node.mesh != INVALID_INDEX)
        {
                drawMesh(shader, meshes[node.mesh], model->meshes[node.mesh], world);
        }

        for (Index childIdx : node.children)
        {
                drawNode(shader, childIdx, world);
        }
}

void GLModel::drawMesh(Shader& shader, GLMesh glMesh, Mesh& mesh, glm::mat4 world)
{

        //  Set model matrix uniform
        shader.setMat4("model", world);

        for (size_t i = 0; i < glMesh.primitives.size(); ++i)
        {
                GLPrimitive& prim       = glMesh.primitives[i];
                MeshPrimitive& meshPrim = mesh.meshPrimitives[i];

                // Bind material if present
                if (meshPrim.material != INVALID_INDEX)
                {
                        Material& mat = model->materials[meshPrim.material];
                        bindMaterial(shader, mat, prim);
                }

                glBindVertexArray(prim.VAO);

                if (prim.draw.count > 0)
                {
                        glDrawElementsBaseVertex(GL_TRIANGLES,
                                                 prim.draw.count,
                                                 prim.indexType,
                                                 (void*)(prim.draw.firstIndex *
                                                         (prim.indexType == GL_UNSIGNED_SHORT ? 2 : 4)),
                                                 prim.draw.baseVertex);
                }
        }
}

void GLModel::bindMaterial(Shader& shader, Material& mat, GLPrimitive& primitive)
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

void GLPrimitive::setupBuffers(Model& model, MeshPrimitive& primitive)
{
        // Determine which attributes are present
        attributeFlags = 0;
        IC_CORE_TRACE("Active vertex attributes:");
        if (primitive.position != INVALID_INDEX)
        {

                IC_CORE_TRACE("  - POSITION");
                attributeFlags |= ATTRIB_POSITION;
        }
        if (primitive.normal != INVALID_INDEX)
        {
                IC_CORE_TRACE("  - NORMAL");
                attributeFlags |= ATTRIB_NORMAL;
        }
        if (!primitive.texcoords.empty() && primitive.texcoords[0] != INVALID_INDEX)
        {
                IC_CORE_TRACE("  - TEXCOORD0");
                attributeFlags |= ATTRIB_TEXCOORD0;
        }
        if (primitive.texcoords.size() > 1 && primitive.texcoords[1] != INVALID_INDEX)
        {
                IC_CORE_TRACE("  - TEXCOORD1");

                attributeFlags |= ATTRIB_TEXCOORD1;
        }
        if (primitive.texcoords.size() > 2 && primitive.texcoords[2] != INVALID_INDEX)
        {
                IC_CORE_TRACE("  - TEXCOORD2");
                attributeFlags |= ATTRIB_TEXCOORD2;
        }

        if (primitive.color != INVALID_INDEX)
        {
                IC_CORE_TRACE("  - COLOR");
                attributeFlags |= ATTRIB_COLOR;
        }
        if (primitive.tangent != INVALID_INDEX)
        {
                IC_CORE_TRACE("  - TANGENT");
                attributeFlags |= ATTRIB_TANGENT;
        }
        if (primitive.joints != INVALID_INDEX)
        {
                IC_CORE_TRACE("  - JOINT");
                attributeFlags |= ATTRIB_JOINTS;
        }
        if (primitive.weights != INVALID_INDEX)
        {
                IC_CORE_TRACE("  - WEIGHT");
                attributeFlags |= ATTRIB_WEIGHTS;
        }

        // Calculate vertex stride based on present attributes
        vertexStride = calculateStride(attributeFlags);
        IC_CORE_TRACE("Vertex Stride: {}", vertexStride);

        // Get vertex count from position accessor
        size_t vertexCount = 0;
        /** TODO: Component type is wrong */
        if (primitive.position != INVALID_INDEX)
        {
                vertexCount = model.accessors[primitive.position].count;
                IC_CORE_TRACE("POSITION0 vertices count: {}", vertexCount);
        }
        else
        {
                IC_CORE_ERROR("Primitive missing position attribute!");
                return;
        }

        // Allocate interleaved vertex buffer
        std::vector<uint8_t> vertexBuffer(vertexCount * vertexStride, 0);

        // Read each attribute into the buffer at its proper offset
        if (attributeFlags & ATTRIB_POSITION)
        {
                readAttribute(model,
                              primitive.position,
                              vertexBuffer,
                              getAttributeOffset(attributeFlags, ATTRIB_POSITION),
                              vertexStride,
                              vertexCount);
        }

        if (attributeFlags & ATTRIB_NORMAL)
        {
                readAttribute(model,
                              primitive.normal,
                              vertexBuffer,
                              getAttributeOffset(attributeFlags, ATTRIB_NORMAL),
                              vertexStride,
                              vertexCount);
        }

        if (attributeFlags & ATTRIB_TEXCOORD0)
        {
                readAttribute(model,
                              primitive.texcoords[0],
                              vertexBuffer,
                              getAttributeOffset(attributeFlags, ATTRIB_TEXCOORD0),
                              vertexStride,
                              vertexCount);
        }

        if (attributeFlags & ATTRIB_TEXCOORD1)
        {
                readAttribute(model,
                              primitive.texcoords[1],
                              vertexBuffer,
                              getAttributeOffset(attributeFlags, ATTRIB_TEXCOORD1),
                              vertexStride,
                              vertexCount);
        }

        if (attributeFlags & ATTRIB_TEXCOORD2)
        {
                readAttribute(model,
                              primitive.texcoords[2],
                              vertexBuffer,
                              getAttributeOffset(attributeFlags, ATTRIB_TEXCOORD2),
                              vertexStride,
                              vertexCount);
        }

        if (attributeFlags & ATTRIB_COLOR)
        {
                readAttribute(model,
                              primitive.color,
                              vertexBuffer,
                              getAttributeOffset(attributeFlags, ATTRIB_COLOR),
                              vertexStride,
                              vertexCount);
        }

        if (attributeFlags & ATTRIB_TANGENT)
        {
                readAttribute(model,
                              primitive.tangent,
                              vertexBuffer,
                              getAttributeOffset(attributeFlags, ATTRIB_TANGENT),
                              vertexStride,
                              vertexCount);
        }

        if (attributeFlags & ATTRIB_JOINTS)
        {
                readAttribute(model,
                              primitive.joints,
                              vertexBuffer,
                              getAttributeOffset(attributeFlags, ATTRIB_JOINTS),
                              vertexStride,
                              vertexCount);
        }

        if (attributeFlags & ATTRIB_WEIGHTS)
        {
                readAttribute(model,
                              primitive.weights,
                              vertexBuffer,
                              getAttributeOffset(attributeFlags, ATTRIB_WEIGHTS),
                              vertexStride,
                              vertexCount);
        }

        // Read indices
        std::vector<uint32_t> indexBuffer;
        if (primitive.indices != INVALID_INDEX)
        {
                readIndices(model, primitive.indices, indexBuffer);
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
        if (!indexBuffer.empty())
        {
                /** TODO: component size */
                glCreateBuffers(1, &EBO);
                glNamedBufferData(EBO, indexBuffer.size() * sizeof(uint32_t), indexBuffer.data(), GL_STATIC_DRAW);
                glVertexArrayElementBuffer(VAO, EBO);

                IC_CORE_TRACE("Uploading EBO: indices={}, indexType=GL_UNSIGNED_INT, totalBytes={}",
                              indexBuffer.size(),
                              indexBuffer.size() * sizeof(uint32_t));

                indexType           = GL_UNSIGNED_INT;

                auto [minIt, maxIt] = std::minmax_element(indexBuffer.begin(), indexBuffer.end());
                IC_CORE_TRACE("Index range: min={}, max={}", *minIt, *maxIt);
                draw.count = static_cast<uint32_t>(indexBuffer.size());
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

void GLPrimitive::readAttribute(Model& model,
                                Index accessorIdx,
                                std::vector<uint8_t>& vertexBuffer,
                                size_t dstOffset,
                                size_t dstStride,
                                size_t vertexCount)
{
        Accessor& acc = model.accessors[accessorIdx];
        IC_CORE_ASSERT(acc.bufferView != INVALID_INDEX, "Accessor has no bufferView!");

        BufferView& view = model.bufferViews[acc.bufferView];
        Buffer& buf      = model.buffers[view.bufferIndex];

        IC_CORE_ASSERT(vertexCount == acc.count, "Vertex count mismatch with accessor!");

        // Determine component size and count based on accessor type
        size_t componentSize  = 0;
        size_t componentCount = 0;

        switch (acc.componentType)
        {
        case GL_UNSIGNED_BYTE:
                componentSize = 1;
                break;
        case GL_UNSIGNED_SHORT:
                componentSize = 2;
                break;
        case GL_UNSIGNED_INT:
                componentSize = 4;
                break;
        case GL_FLOAT:
                componentSize = 4;
                break;
        default:
                IC_CORE_ERROR("Unsupported component type: {}", acc.componentType);
                return;
        }

        switch (acc.type)
        {
        case Accessor::Type::SCALAR:
                componentCount = 1;
                break;
        case Accessor::Type::VEC2:
                componentCount = 2;
                break;
        case Accessor::Type::VEC3:
                componentCount = 3;
                break;
        case Accessor::Type::VEC4:
                componentCount = 4;
                break;
        case Accessor::Type::MAT4:
                componentCount = 16;
                break;
        default:
                IC_CORE_ERROR("Unsupported accessor type!");
                return;
        }

        // stride calculation
        const size_t elementSize = componentSize * componentCount;
        size_t sourceStride      = view.byteStride != 0 ? view.byteStride : elementSize;

        IC_CORE_ASSERT(sourceStride >= elementSize, "Invalid bufferView stride!");

        // read data
        const size_t accessorStart = view.byteOffset + acc.offset;
        const size_t accessorEnd   = accessorStart + (acc.count - 1) * sourceStride + elementSize;

        IC_CORE_ASSERT(accessorEnd <= view.byteOffset + view.byteLength, "Accessor reads past end of bufferView!");
        IC_CORE_ASSERT(accessorEnd <= buf.data.size(), "Accessor reads past end of buffer!");

        IC_CORE_TRACE("Reading vertices: count={}, componentSize={}, stride={}, bufferView=[{}, {})",
                      acc.count,
                      componentSize,
                      sourceStride,
                      accessorStart,
                      accessorEnd);

        const uint8_t* base = buf.data.data() + accessorStart;

        for (size_t i = 0; i < acc.count; ++i)
        {
                const uint8_t* src = base + i * sourceStride;
                uint8_t* dst       = vertexBuffer.data() + i * dstStride + dstOffset;

                memcpy(dst, src, elementSize);
        }

        IC_CORE_TRACE("Finished reading {} vertices", vertexBuffer.size());
}

void GLPrimitive::readIndices(Model& model, Index accessorIdx, std::vector<uint32_t>& indexBuffer)
{
        Accessor& acc = model.accessors[accessorIdx];

        IC_CORE_ASSERT(acc.bufferView != INVALID_INDEX, "Index accessor has no bufferView!");
        IC_CORE_ASSERT(acc.count > 0, "Index accessor has zero count!");

        BufferView& view = model.bufferViews[acc.bufferView];
        Buffer& buf      = model.buffers[view.bufferIndex];

        // ---- Determine component size ----
        size_t componentSize = 0;
        switch (acc.componentType)
        {
        case GL_UNSIGNED_BYTE:
                componentSize = 1;
                break;
        case GL_UNSIGNED_SHORT:
                componentSize = 2;
                break;
        case GL_UNSIGNED_INT:
                componentSize = 4;
                break;
        default:
                IC_CORE_ERROR("Invalid index component type: {}", acc.componentType);
                return;
        }

        // ---- Source stride ----
        size_t sourceStride = view.byteStride != 0 ? view.byteStride : componentSize;
        IC_CORE_ASSERT(sourceStride >= componentSize, "Invalid index bufferView stride!");

        // ---- Compute read range ----
        const size_t accessorStart = view.byteOffset + acc.offset;

        const size_t accessorEnd   = accessorStart + (acc.count - 1) * sourceStride + componentSize;

        IC_CORE_ASSERT(accessorEnd <= view.byteOffset + view.byteLength,
                       "Index accessor reads past end of bufferView!");

        IC_CORE_ASSERT(accessorEnd <= buf.data.size(), "Index accessor reads past end of buffer!");

        // ---- Resize output ----
        indexBuffer.resize(acc.count);

        IC_CORE_TRACE("Reading indices: count={}, componentSize={}, stride={}, bufferView=[{}, {})",
                      acc.count,
                      componentSize,
                      sourceStride,
                      accessorStart,
                      accessorEnd);

        // ---- Read indices ----
        const uint8_t* base = buf.data.data() + accessorStart;

        for (size_t i = 0; i < acc.count; ++i)
        {
                const uint8_t* src = base + i * sourceStride;

                switch (acc.componentType)
                {
                case GL_UNSIGNED_BYTE:
                        indexBuffer[i] = static_cast<uint32_t>(*src);
                        break;

                case GL_UNSIGNED_SHORT:
                        indexBuffer[i] = static_cast<uint32_t>(*reinterpret_cast<const uint16_t*>(src));
                        break;

                case GL_UNSIGNED_INT:
                        indexBuffer[i] = *reinterpret_cast<const uint32_t*>(src);
                        break;
                }
        }

        IC_CORE_TRACE("Finished reading {} indices", indexBuffer.size());
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
/** TODO: write this function better */
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