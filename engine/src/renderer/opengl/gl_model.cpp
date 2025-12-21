#include "gl_model.h"

namespace ic
{

/** Plan:
 * Scenes Traverse the nodes call the function for each index to load a node from there respective index in the
 * data in all the vectors given. Now each node contains children, localTransform and the node itself is either
 * references a mesh a skin or a model Now we want is hierarchy of things going on we just dont want things to look like
 * they are not in our hands so all the node hierarchy stuff is going to be done with the Model struct that I created
 * earlier . Along with each mesh comes its material. The material data comes from the texture data each texture has a
 * source image attached to it. To load a mesh we will first load its vertex data everything int the indices and
 * vertices position, normals etc. After that we will load for each mesh''s primitive its material. Now a material
 * requires a texture (gotta improve its struct) and each texture contains a sampler and a data. We are goint to load
 * all that data into the GPU memory with whatever the hell we do it in OPENGL way and yeah we have got ourselves a
 * simple model loader and we will be done with it. Now the problem is probably writing the tests and everything. I am
 * probably going to write all the tests starting with the filesystem that I am going to create. After we are done with
 * that i am going to start and ECS system and actually start creating games after actually completing my UNITY tutorial
 * FFS.
 */

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
        }
}

void GLModel::uploadMeshes()
{
        meshes.resize(model->meshes.size());

        for (size_t i = 0; i < model->meshes.size(); ++i)
        {
                Mesh& mesh = model->meshes[i];

                meshes[i].primitives.resize(mesh.meshPrimitives.size());

                for (size_t j = 0; j < mesh.meshPrimitives.size(); ++j)
                {
                        meshes[i].primitives[j].setupBuffers(*model, mesh.meshPrimitives[j]);
                }
        }
}

void GLModel::draw()
{
        ic::Scene& scene = model->scenes[model->defaultScene];

        for (auto& rootNodeIdx : scene.rootNodes)
        {
                drawNode(rootNodeIdx, glm::mat4(1.0f));  //** Transform needs to be checked. */
        }
}

void GLModel::drawNode(Index idx, glm::mat4 parent)
{
        Node& node      = model->nodes[idx];
        glm::mat4 world = parent * node.localTransform;

        if (node.mesh != INVALID_INDEX)
        {
                drawMesh(meshes[node.mesh], model->meshes[node.mesh], world);
        }

        for (Index childIdx : node.children)
        {
                drawNode(childIdx, world);
        }
}

void GLModel::drawMesh(GLMesh glMesh, Mesh& mesh, glm::mat4 world)
{

        //  Set model matrix uniform
        // setUniform("u_Model", world);

        for (size_t i = 0; i < glMesh.primitives.size(); ++i)
        {
                GLPrimitive& prim       = glMesh.primitives[i];
                MeshPrimitive& meshPrim = mesh.meshPrimitives[i];

                // Bind material if present
                if (meshPrim.material != INVALID_INDEX)
                {
                        // Material& mat = model->materials[meshPrim.material];
                        // bindMaterial(mat);
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

void GLTexture::createTexture(Model& model, Texture& tex, ImageData& img)
{
        glCreateTextures(GL_TEXTURE_2D, 1, &textureHandle);

        // Determine internal format
        GLenum internalFormat = GL_RGBA8;
        GLenum format         = GL_RGBA;

        if (img.srgb)
        {
                internalFormat = GL_SRGB8_ALPHA8;
        }

        // Allocate storage
        glTextureStorage2D(textureHandle, 1, internalFormat, img.width, img.height);

        // Upload pixel data
        glTextureSubImage2D(textureHandle, 0, 0, 0, img.width, img.height, format, GL_UNSIGNED_BYTE, img.pixels.data());

        // Set sampler parameters
        if (tex.sampler != INVALID_INDEX)
        {
                Sampler& sampler = model.samplers[tex.sampler];

                glTextureParameteri(textureHandle, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(sampler.minFilter));
                glTextureParameteri(textureHandle, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(sampler.magFilter));
                glTextureParameteri(textureHandle, GL_TEXTURE_WRAP_S, static_cast<GLint>(sampler.wrapS));
                glTextureParameteri(textureHandle, GL_TEXTURE_WRAP_T, static_cast<GLint>(sampler.wrapT));
        }
        else
        {
                // Default sampler settings
                glTextureParameteri(textureHandle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTextureParameteri(textureHandle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTextureParameteri(textureHandle, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTextureParameteri(textureHandle, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }

        // Generate mipmaps
        glGenerateTextureMipmap(textureHandle);
}

void GLPrimitive::setupBuffers(Model& model, MeshPrimitive& primitive)
{
        // Determine which attributes are present
        attributeFlags = 0;
        if (primitive.position != INVALID_INDEX)
                attributeFlags |= ATTRIB_POSITION;
        if (primitive.normal != INVALID_INDEX)
                attributeFlags |= ATTRIB_NORMAL;
        if (primitive.tangent != INVALID_INDEX)
                attributeFlags |= ATTRIB_TANGENT;
        if (primitive.color != INVALID_INDEX)
                attributeFlags |= ATTRIB_COLOR;
        if (primitive.joints != INVALID_INDEX)
                attributeFlags |= ATTRIB_JOINTS;
        if (primitive.weights != INVALID_INDEX)
                attributeFlags |= ATTRIB_WEIGHTS;

        if (!primitive.texcoords.empty() && primitive.texcoords[0] != INVALID_INDEX)
                attributeFlags |= ATTRIB_TEXCOORD0;
        if (primitive.texcoords.size() > 1 && primitive.texcoords[1] != INVALID_INDEX)
                attributeFlags |= ATTRIB_TEXCOORD1;

        // Calculate vertex stride based on present attributes
        vertexStride = calculateStride(attributeFlags);

        // Get vertex count from position accessor
        size_t vertexCount = 0;
        if (primitive.position != INVALID_INDEX)
        {
                vertexCount = model.accessors[primitive.position].count;
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

        if (attributeFlags & ATTRIB_TANGENT)
        {
                readAttribute(model,
                              primitive.tangent,
                              vertexBuffer,
                              getAttributeOffset(attributeFlags, ATTRIB_TANGENT),
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

        if (attributeFlags & ATTRIB_COLOR)
        {
                readAttribute(model,
                              primitive.color,
                              vertexBuffer,
                              getAttributeOffset(attributeFlags, ATTRIB_COLOR),
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

        // Upload index data if present
        if (!indexBuffer.empty())
        {
                glCreateBuffers(1, &EBO);
                glNamedBufferData(EBO, indexBuffer.size() * sizeof(uint32_t), indexBuffer.data(), GL_STATIC_DRAW);
                glVertexArrayElementBuffer(VAO, EBO);

                indexType  = GL_UNSIGNED_INT;
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
}

void GLPrimitive::readAttribute(Model& model,
                                Index accessorIdx,
                                std::vector<uint8_t>& vertexBuffer,
                                size_t offset,
                                size_t stride,
                                size_t vertexCount)
{
        Accessor& acc       = model.accessors[accessorIdx];
        BufferView& view    = model.bufferViews[acc.bufferView];
        Buffer& buf         = model.buffers[view.buffer];

        const uint8_t* base = buf.data.data() + view.offset + acc.offset;
        size_t sourceStride = view.stride ? view.stride : 0;

        // Determine component size and count based on accessor type
        size_t componentSize  = 0;
        size_t componentCount = 0;

        switch (acc.componentType)
        {
        case GL_BYTE:
        case GL_UNSIGNED_BYTE:
                componentSize = 1;
                break;
        case GL_SHORT:
        case GL_UNSIGNED_SHORT:
                componentSize = 2;
                break;
        case GL_UNSIGNED_INT:
        case GL_FLOAT:
                componentSize = 4;
                break;
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
                componentCount = 1;
                break;
        }

        size_t elementSize = componentSize * componentCount;
        if (sourceStride == 0)
                sourceStride = elementSize;

        // Copy data into interleaved buffer
        for (size_t i = 0; i < vertexCount; ++i)
        {
                const uint8_t* src = base + i * sourceStride;
                uint8_t* dst       = vertexBuffer.data() + i * stride + offset;
                std::memcpy(dst, src, elementSize);
        }
}

void GLPrimitive::readIndices(Model& model, Index accessorIdx, std::vector<uint32_t>& indexBuffer)
{
        Accessor& acc       = model.accessors[accessorIdx];
        BufferView& view    = model.bufferViews[acc.bufferView];
        Buffer& buf         = model.buffers[view.buffer];

        const uint8_t* base = buf.data.data() + view.offset + acc.offset;
        indexBuffer.resize(acc.count);

        // Convert indices to uint32_t
        for (size_t i = 0; i < acc.count; ++i)
        {
                switch (acc.componentType)
                {
                case GL_UNSIGNED_BYTE:
                        indexBuffer[i] = static_cast<uint32_t>(base[i]);
                        break;
                case GL_UNSIGNED_SHORT:
                {
                        const uint16_t* indices16 = reinterpret_cast<const uint16_t*>(base);
                        indexBuffer[i]            = static_cast<uint32_t>(indices16[i]);
                        break;
                }
                case GL_UNSIGNED_INT:
                {
                        const uint32_t* indices32 = reinterpret_cast<const uint32_t*>(base);
                        indexBuffer[i]            = indices32[i];
                        break;
                }
                }
        }
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

        if (attributeFlags & ATTRIB_TANGENT)
        {
                glEnableVertexArrayAttrib(VAO, attribIndex);
                glVertexArrayAttribFormat(
                    VAO, attribIndex, 4, GL_FLOAT, GL_FALSE, getAttributeOffset(attributeFlags, ATTRIB_TANGENT));
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

        if (attributeFlags & ATTRIB_COLOR)
        {
                glEnableVertexArrayAttrib(VAO, attribIndex);
                glVertexArrayAttribFormat(
                    VAO, attribIndex, 4, GL_FLOAT, GL_FALSE, getAttributeOffset(attributeFlags, ATTRIB_COLOR));
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
        if (flags & ATTRIB_TANGENT)
                stride += sizeof(glm::vec4);
        if (flags & ATTRIB_TEXCOORD0)
                stride += sizeof(glm::vec2);
        if (flags & ATTRIB_TEXCOORD1)
                stride += sizeof(glm::vec2);
        if (flags & ATTRIB_COLOR)
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