#include "model.h"
#include "buffer.h"

VkDescriptorSetLayout vkLoad::descriptorSetLayoutImage = VK_NULL_HANDLE;
VkMemoryPropertyFlags vkLoad::memoryPropertyFlags      = 0;
VkDescriptorSetLayout vkLoad::descriptorSetLayoutUbo   = VK_NULL_HANDLE;
namespace vkLoad
{

        bool loadImageDataFunc(tinygltf::Image* image,
                               const int imageIndex,
                               std::string* error,
                               std::string* warning,
                               int req_width,
                               int req_height,
                               const unsigned char* bytes,
                               int size,
                               void* userData)
        {
                // KTX files will be handled by our own code
                if (image->uri.find_last_of(".") != std::string::npos)
                {
                        if (image->uri.substr(image->uri.find_last_of(".") + 1) == "ktx2")
                        {
                                return true;
                        }
                }

                return tinygltf::LoadImageData(
                    image, imageIndex, error, warning, req_width, req_height, bytes, size, userData);
        }

        bool loadImageDataFuncEmpty(tinygltf::Image* image,
                                    const int imageIndex,
                                    std::string* error,
                                    std::string* warning,
                                    int req_width,
                                    int req_height,
                                    const unsigned char* bytes,
                                    int size,
                                    void* userData)
        {
                // This function will be used for samples that don't require images to be loaded
                return true;
        }

        BoundingBox::BoundingBox() {}
        BoundingBox::BoundingBox(glm::vec3 min, glm::vec3 max) : min(min), max(max) {}

        BoundingBox BoundingBox::getAABB(glm::mat4 transformMatrix)
        {
                // get the translation for position
                glm::vec3 min = glm::vec3(transformMatrix[3]);
                glm::vec3 max = min;
                glm::vec3 v0, v1;

                // get the x axis using first column
                glm::vec3 right  = glm::vec3(transformMatrix[0]);
                v0               = right * this->min.x;
                v1               = right * this->max.x;
                min             += glm::min(v0, v1);
                max             += glm::max(v0, v1);

                // get the y axis
                glm::vec3 up  = glm::vec3(transformMatrix[1]);
                v0            = up * this->min.y;
                v1            = up * this->max.y;
                min          += glm::min(v0, v1);
                max          += glm::max(v0, v1);

                // get the z axis
                glm::vec3 back  = glm::vec3(transformMatrix[2]);
                v0              = back * this->min.z;
                v1              = back * this->max.z;
                min            += glm::min(v0, v1);
                max            += glm::max(v0, v1);

                return BoundingBox(min, max);
        }

        void Material::createDescriptorSet(VkDescriptorPool descriptorPool,
                                           VkDescriptorSetLayout setLayout,
                                           uint32_t descriptorBindingFlags)
        {
                VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
                descriptorSetAllocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                descriptorSetAllocInfo.descriptorPool     = descriptorPool;
                descriptorSetAllocInfo.descriptorSetCount = 1;
                descriptorSetAllocInfo.pSetLayouts        = &setLayout;

                IC_CORE_ASSERT(vkAllocateDescriptorSets(device->logicalDevice,
                                                        &descriptorSetAllocInfo,
                                                        &descriptorSet) == VK_SUCCESS,
                               "Failed to allocate descriptors");

                std::vector<VkDescriptorImageInfo> imageDescriptors{};
                std::vector<VkWriteDescriptorSet> writeDescriptorSets{};

                if (descriptorBindingFlags & DescriptorBindingFlags::ImageBaseColor)
                {
                        imageDescriptors.push_back(baseColorTexture->descriptor);
                        VkWriteDescriptorSet writeDescriptorSet{};
                        writeDescriptorSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        writeDescriptorSet.dstSet          = descriptorSet;
                        writeDescriptorSet.dstBinding      = 1;
                        writeDescriptorSet.descriptorCount = 1;
                        writeDescriptorSet.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                        writeDescriptorSet.pImageInfo      = &baseColorTexture->descriptor;
                        writeDescriptorSets.push_back(writeDescriptorSet);
                }
                if (normalTexture && descriptorBindingFlags & DescriptorBindingFlags::ImageNormalMap)
                {
                        imageDescriptors.push_back(normalTexture->descriptor);
                        VkWriteDescriptorSet writeDescriptorSet{};
                        writeDescriptorSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        writeDescriptorSet.dstSet          = descriptorSet;
                        writeDescriptorSet.dstBinding      = 1;
                        writeDescriptorSet.descriptorCount = 1;
                        writeDescriptorSet.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                        writeDescriptorSet.pImageInfo      = &normalTexture->descriptor;
                        writeDescriptorSets.push_back(writeDescriptorSet);
                }
                vkUpdateDescriptorSets(device->logicalDevice,
                                       static_cast<uint32_t>(writeDescriptorSets.size()),
                                       writeDescriptorSets.data(),
                                       0,
                                       nullptr);
        }

        void Primitive::setBoundingBox(glm::vec3 min, glm::vec3 max)
        {
                bb.max = max;
                bb.min = min;
        }

        Mesh::Mesh(ic::vkdevice* device, glm::mat4 matrix) : device(device)
        {
                this->uniformBlock.matrix = matrix;
                IC_CORE_ASSERT(device->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                                    sizeof(uniformBlock),
                                                    &uniformBuffer.buffer,
                                                    &uniformBuffer.memory,
                                                    &uniformBlock) == VK_SUCCESS,
                               "Failed to create Buffer");

                IC_CORE_ASSERT(vkMapMemory(device->logicalDevice,
                                           uniformBuffer.memory,
                                           0,
                                           sizeof(uniformBlock),
                                           0,
                                           &uniformBuffer.mapped) == VK_SUCCESS,
                               "Failed to allocate buffer to the mesh.");
                uniformBuffer.descriptor.buffer = uniformBuffer.buffer;
                uniformBuffer.descriptor.offset = 0;
                uniformBuffer.descriptor.range  = sizeof(uniformBlock);
        }

        Mesh::~Mesh()
        {
                vkDestroyBuffer(device->logicalDevice, uniformBuffer.buffer, nullptr);
                vkFreeMemory(device->logicalDevice, uniformBuffer.memory, nullptr);
                for (auto primitive : primitives)
                {
                        delete primitive;
                }
        }

        void Mesh::setBoundingBox(glm::vec3 min, glm::vec3 max)
        {
                bb.max   = max;
                bb.min   = min;
                bb.valid = true;
                // aabb = bb.getAABB()
        }

        glm::mat4 Node::localMatrix()
        {
                if (!useCachedMatrix)
                {
                        cachedLocalMatrix = glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) *
                                            glm::scale(glm::mat4(1.0f), scale) * matrix;
                }
                return cachedLocalMatrix;
        }

        glm::mat4 Node::getMatrix()
        {
                // Use a simple caching algorithm to avoid having to recalculate matrices too often while traversing the
                // node hierarchy
                if (!useCachedMatrix)
                {
                        glm::mat4 m     = localMatrix();
                        vkLoad::Node* p = parent;
                        while (p)
                        {
                                m = p->localMatrix() * m;
                                p = p->parent;
                        }
                        cachedMatrix    = m;
                        useCachedMatrix = true;
                        return m;
                }
                else
                {
                        return cachedMatrix;
                }
        }

        void Node::update()
        {
                useCachedMatrix = false;

                if (mesh)
                {
                        glm::mat4 m = getMatrix();
                        if (skin)
                        {
                                // if the mesh has a skin update the skin using the animations
                                mesh->uniformBlock.matrix = m;
                                glm::mat4 invTransform    = glm::inverse(m);
                                size_t numJoints          = std::min((uint32_t)skin->joints.size(), MAX_NUM_JOINTS);

                                for (size_t i = 0; i < numJoints; i++)
                                {
                                        vkLoad::Node* jointNode = skin->joints[i];
                                        glm::mat4 jointMat = jointNode->getMatrix() * skin[i].inverseBindMatrices[i];
                                        jointMat           = invTransform * jointMat;
                                        mesh->uniformBlock.jointMatrix[i] = jointMat;
                                }
                                mesh->uniformBlock.jointcount = static_cast<uint32_t>(numJoints);
                        }
                        else
                        {
                                mesh->uniformBlock.matrix = m;
                        }
                }

                for (auto& child : children)
                {
                        child->update();
                }
        }

        Node::~Node()
        {
                if (mesh)
                {
                        delete mesh;
                }
                for (auto& child : children)
                {
                        delete child;
                }
        }

        // Cube spline interpolation function used for translate/scale/rotate with cubic spline animation samples
        // Details on how this works can be found in the specs
        // https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#appendix-c-spline-interpolation
        glm::vec4 AnimationSampler::cubicSplineInterpolation(size_t index, float time, uint32_t stride)
        {
                float delta          = inputs[index + 1] - inputs[index];
                float t              = (time - inputs[index]) / delta;
                const size_t current = index * stride * 3;
                const size_t next    = (index + 1) * stride * 3;
                const size_t A       = 0;
                const size_t V       = stride * 1;
                const size_t B       = stride * 2;

                float t2             = powf(t, 2);
                float t3             = powf(t, 3);
                glm::vec4 pt{0.0f};
                for (uint32_t i = 0; i < stride; i++)
                {
                        float p0 = outputs[current + i + V];          // starting point at t = 0
                        float m0 = delta * outputs[current + i + A];  // scaled starting tangent at t = 0
                        float p1 = outputs[next + i + V];             // ending point at t = 1
                        float m1 = delta * outputs[next + i + B];     // scaled ending tangent at t = 1
                        pt[i]    = ((2.f * t3 - 3.f * t2 + 1.f) * p0) + ((t3 - 2.f * t2 + t) * m0) +
                                ((-2.f * t3 + 3.f * t2) * p1) + ((t3 - t2) * m0);
                }
                return pt;
        }

        // Calculates the translation of this sampler for the given node at a given time point depending on the
        // interpolation type
        void AnimationSampler::translate(size_t index, float time, vkLoad::Node* node)
        {
                switch (interpolation)
                {
                case AnimationSampler::InterpolationType::LINEAR:
                {
                        float u           = std::max(0.0f, time - inputs[index]) / (inputs[index + 1] - inputs[index]);
                        node->translation = glm::mix(outputsVec4[index], outputsVec4[index + 1], u);
                        break;
                }
                case AnimationSampler::InterpolationType::STEP:
                {
                        node->translation = outputsVec4[index];
                        break;
                }
                case AnimationSampler::InterpolationType::CUBICSPLINE:
                {
                        node->translation = cubicSplineInterpolation(index, time, 3);
                        break;
                }
                }
        }

        // Calculates the scale of this sampler for the given node at a given time point depending on the interpolation
        // type
        void AnimationSampler::scale(size_t index, float time, vkLoad::Node* node)
        {
                switch (interpolation)
                {
                case AnimationSampler::InterpolationType::LINEAR:
                {
                        float u     = std::max(0.0f, time - inputs[index]) / (inputs[index + 1] - inputs[index]);
                        node->scale = glm::mix(outputsVec4[index], outputsVec4[index + 1], u);
                        break;
                }
                case AnimationSampler::InterpolationType::STEP:
                {
                        node->scale = outputsVec4[index];
                        break;
                }
                case AnimationSampler::InterpolationType::CUBICSPLINE:
                {
                        node->scale = cubicSplineInterpolation(index, time, 3);
                        break;
                }
                }
        }

        // Calculates the rotation of this sampler for the given node at a given time point depending on the
        // interpolation type
        void AnimationSampler::rotate(size_t index, float time, vkLoad::Node* node)
        {
                switch (interpolation)
                {
                case AnimationSampler::InterpolationType::LINEAR:
                {
                        float u = std::max(0.0f, time - inputs[index]) / (inputs[index + 1] - inputs[index]);
                        glm::quat q1;
                        q1.x = outputsVec4[index].x;
                        q1.y = outputsVec4[index].y;
                        q1.z = outputsVec4[index].z;
                        q1.w = outputsVec4[index].w;
                        glm::quat q2;
                        q2.x           = outputsVec4[index + 1].x;
                        q2.y           = outputsVec4[index + 1].y;
                        q2.z           = outputsVec4[index + 1].z;
                        q2.w           = outputsVec4[index + 1].w;
                        node->rotation = glm::normalize(glm::slerp(q1, q2, u));
                        break;
                }
                case AnimationSampler::InterpolationType::STEP:
                {
                        glm::quat q1;
                        q1.x           = outputsVec4[index].x;
                        q1.y           = outputsVec4[index].y;
                        q1.z           = outputsVec4[index].z;
                        q1.w           = outputsVec4[index].w;
                        node->rotation = q1;
                        break;
                }
                case AnimationSampler::InterpolationType::CUBICSPLINE:
                {
                        glm::vec4 rot = cubicSplineInterpolation(index, time, 4);
                        glm::quat q;
                        q.x            = rot.x;
                        q.y            = rot.y;
                        q.z            = rot.z;
                        q.w            = rot.w;
                        node->rotation = glm::normalize(q);
                        break;
                }
                }
        }

        void Model::destroy(VkDevice device)
        {
                if (vertices.buffer != VK_NULL_HANDLE)
                {
                        vkDestroyBuffer(device, vertices.buffer, nullptr);
                        vkFreeMemory(device, vertices.memory, nullptr);
                        vertices.buffer = VK_NULL_HANDLE;
                }
                if (indices.buffer != VK_NULL_HANDLE)
                {
                        vkDestroyBuffer(device, indices.buffer, nullptr);
                        vkFreeMemory(device, indices.memory, nullptr);
                        indices.buffer = VK_NULL_HANDLE;
                }

                if (descriptorSetLayoutImage != VK_NULL_HANDLE)
                {
                        vkDestroyDescriptorSetLayout(device, descriptorSetLayoutImage, nullptr);
                }

                if (descriptorSetLayoutUbo != VK_NULL_HANDLE)
                {
                        vkDestroyDescriptorSetLayout(device, descriptorSetLayoutUbo, nullptr);
                }

                if (descriptorPool != VK_NULL_HANDLE)
                {
                        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
                }
                for (auto texture : textures)
                {
                        texture.destroy();
                }
                textures.clear();
                textureSamplers.clear();
                for (auto node : nodes)
                {
                        delete node;
                }
                materials.clear();
                animations.clear();
                nodes.clear();
                linearNodes.clear();
                extensions.clear();
                for (auto skin : skins)
                {
                        delete skin;
                }
                skins.clear();
        }

        void Model::loadNode(vkLoad::Node* parent,
                             const tinygltf::Node& node,
                             uint32_t nodeIndex,
                             const tinygltf::Model& model,
                             LoaderInfo& loaderInfo,
                             float globalscale)
        {
                vkLoad::Node* newNode = new Node();
                newNode->index        = nodeIndex;
                newNode->parent       = parent;
                newNode->name         = node.name;
                newNode->skinIndex    = node.skin;
                newNode->matrix       = glm::mat4(1.0f);

                IC_CORE_INFO("Loading Mesh Node: {}", node.name);
                // generate local node matrix
                glm::vec3 translation = glm::vec3(0.0f);
                if (node.translation.size() == 3)
                {
                        translation          = glm::make_vec3(node.translation.data());
                        newNode->translation = translation;
                }
                glm::mat4 rotation = glm::mat4(1.0f);
                if (node.rotation.size() == 4)
                {
                        glm::quat q       = glm::make_quat(node.rotation.data());
                        newNode->rotation = glm::mat4(q);
                }
                glm::vec3 scale = glm::vec3(1.0f);
                if (node.scale.size() == 3)
                {
                        scale          = glm::make_vec3(node.scale.data());
                        newNode->scale = scale;
                }
                if (node.matrix.size() == 16)
                {
                        newNode->matrix = glm::make_mat4x4(node.matrix.data());
                }

                // Node with children
                if (node.children.size() > 0)
                {
                        for (size_t i = 0; i < node.children.size(); i++)
                        {
                                loadNode(newNode,
                                         model.nodes[node.children[i]],
                                         node.children[i],
                                         model,
                                         loaderInfo,
                                         globalscale);
                        }
                }
                IC_CORE_INFO("For Node {} found:", node.name);
                // Node contains mesh data
                if (node.mesh > -1)
                {
                        const tinygltf::Mesh mesh = model.meshes[node.mesh];
                        Mesh* newMesh             = new Mesh(device, newNode->matrix);
                        for (size_t j = 0; j < mesh.primitives.size(); j++)
                        {
                                const tinygltf::Primitive& primitive = mesh.primitives[j];
                                uint32_t vertexStart                 = static_cast<uint32_t>(loaderInfo.vertexPos);
                                uint32_t indexStart                  = static_cast<uint32_t>(loaderInfo.indexPos);
                                uint32_t indexCount                  = 0;
                                uint32_t vertexCount                 = 0;
                                glm::vec3 posMin{};
                                glm::vec3 posMax{};
                                bool hasSkin    = false;
                                bool hasIndices = primitive.indices > -1;
                                // Vertices
                                {
                                        const float* bufferPos          = nullptr;
                                        const float* bufferNormals      = nullptr;
                                        const float* bufferTexCoordSet0 = nullptr;
                                        const float* bufferTexCoordSet1 = nullptr;
                                        const float* bufferColorSet0    = nullptr;
                                        const void* bufferJoints        = nullptr;
                                        const float* bufferWeights      = nullptr;

                                        int posByteStride;
                                        int normByteStride;
                                        int uv0ByteStride;
                                        int uv1ByteStride;
                                        int color0ByteStride;
                                        int jointByteStride;
                                        int weightByteStride;

                                        int jointComponentType;

                                        // Position attribute is required
                                        IC_CORE_ASSERT(primitive.attributes.find("POSITION") !=
                                                           primitive.attributes.end(),
                                                       "    Couldn't Find POSITION attribute for the primitive {}",
                                                       node.name);

                                        const tinygltf::Accessor& posAccessor =
                                            model.accessors[primitive.attributes.find("POSITION")->second];
                                        const tinygltf::BufferView& posView = model.bufferViews[posAccessor.bufferView];
                                        bufferPos                           = reinterpret_cast<const float*>(
                                            &(model.buffers[posView.buffer]
                                                  .data[posAccessor.byteOffset + posView.byteOffset]));
                                        posMin        = glm::vec3(posAccessor.minValues[0],
                                                           posAccessor.minValues[1],
                                                           posAccessor.minValues[2]);
                                        posMax        = glm::vec3(posAccessor.maxValues[0],
                                                           posAccessor.maxValues[1],
                                                           posAccessor.maxValues[2]);
                                        vertexCount   = static_cast<uint32_t>(posAccessor.count);
                                        posByteStride = posAccessor.ByteStride(posView)
                                                            ? (posAccessor.ByteStride(posView) / sizeof(float))
                                                            : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
                                        IC_CORE_INFO("    Found {} vertex Position_0s with stride {}",
                                                     vertexCount,
                                                     posByteStride);

                                        if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
                                        {
                                                IC_CORE_INFO("    Model contains Normal attributes!");
                                                const tinygltf::Accessor& normAccessor =
                                                    model.accessors[primitive.attributes.find("NORMAL")->second];
                                                const tinygltf::BufferView& normView =
                                                    model.bufferViews[normAccessor.bufferView];
                                                bufferNormals = reinterpret_cast<const float*>(
                                                    &(model.buffers[normView.buffer]
                                                          .data[normAccessor.byteOffset + normView.byteOffset]));
                                                normByteStride =
                                                    normAccessor.ByteStride(normView)
                                                        ? (normAccessor.ByteStride(normView) / sizeof(float))
                                                        : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
                                                IC_CORE_INFO("    Found Normal stride {} for normal buffer",
                                                             normByteStride);
                                        }

                                        // UVs
                                        if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
                                        {
                                                IC_CORE_INFO("    Model contains Texture_0 attributes!");
                                                const tinygltf::Accessor& uvAccessor =
                                                    model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
                                                const tinygltf::BufferView& uvView =
                                                    model.bufferViews[uvAccessor.bufferView];
                                                bufferTexCoordSet0 = reinterpret_cast<const float*>(
                                                    &(model.buffers[uvView.buffer]
                                                          .data[uvAccessor.byteOffset + uvView.byteOffset]));
                                                uv0ByteStride = uvAccessor.ByteStride(uvView)
                                                                    ? (uvAccessor.ByteStride(uvView) / sizeof(float))
                                                                    : tinygltf::GetNumComponentsInType(
                                                                          TINYGLTF_TYPE_VEC2);
                                                IC_CORE_INFO("    Found Texture_0 attrib of stride {}", uv0ByteStride);
                                        }
                                        if (primitive.attributes.find("TEXCOORD_1") != primitive.attributes.end())
                                        {
                                                IC_CORE_INFO("    Model contains Texture_1 attributes!");
                                                const tinygltf::Accessor& uvAccessor =
                                                    model.accessors[primitive.attributes.find("TEXCOORD_1")->second];
                                                const tinygltf::BufferView& uvView =
                                                    model.bufferViews[uvAccessor.bufferView];
                                                bufferTexCoordSet1 = reinterpret_cast<const float*>(
                                                    &(model.buffers[uvView.buffer]
                                                          .data[uvAccessor.byteOffset + uvView.byteOffset]));
                                                uv1ByteStride = uvAccessor.ByteStride(uvView)
                                                                    ? (uvAccessor.ByteStride(uvView) / sizeof(float))
                                                                    : tinygltf::GetNumComponentsInType(
                                                                          TINYGLTF_TYPE_VEC2);
                                                IC_CORE_INFO("    Found Texture_0 attrib of stride {}", uv1ByteStride);
                                        }

                                        // Vertex colors
                                        if (primitive.attributes.find("COLOR_0") != primitive.attributes.end())
                                        {
                                                IC_CORE_INFO("    Model contains Color_0 attributes");
                                                const tinygltf::Accessor& accessor =
                                                    model.accessors[primitive.attributes.find("COLOR_0")->second];
                                                const tinygltf::BufferView& view =
                                                    model.bufferViews[accessor.bufferView];
                                                bufferColorSet0 = reinterpret_cast<const float*>(
                                                    &(model.buffers[view.buffer]
                                                          .data[accessor.byteOffset + view.byteOffset]));
                                                color0ByteStride = accessor.ByteStride(view)
                                                                       ? (accessor.ByteStride(view) / sizeof(float))
                                                                       : tinygltf::GetNumComponentsInType(
                                                                             TINYGLTF_TYPE_VEC3);
                                                IC_CORE_INFO("    Found Color_0 attrib of stride {}", color0ByteStride);
                                        }

                                        // Skinning
                                        // Joints
                                        if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end())
                                        {
                                                IC_CORE_INFO("    Model contains Joint_0 attributes");
                                                const tinygltf::Accessor& jointAccessor =
                                                    model.accessors[primitive.attributes.find("JOINTS_0")->second];
                                                const tinygltf::BufferView& jointView =
                                                    model.bufferViews[jointAccessor.bufferView];
                                                bufferJoints = &(
                                                    model.buffers[jointView.buffer]
                                                        .data[jointAccessor.byteOffset + jointView.byteOffset]);
                                                jointComponentType = jointAccessor.componentType;
                                                jointByteStride =
                                                    jointAccessor.ByteStride(jointView)
                                                        ? (jointAccessor.ByteStride(jointView) /
                                                           tinygltf::GetComponentSizeInBytes(jointComponentType))
                                                        : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC4);
                                                IC_CORE_INFO("    Found Joint_0 attrib of stride {}", jointByteStride);
                                        }

                                        if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end())
                                        {
                                                IC_CORE_INFO("    Model contains Weight_0 attributes");
                                                const tinygltf::Accessor& weightAccessor =
                                                    model.accessors[primitive.attributes.find("WEIGHTS_0")->second];
                                                const tinygltf::BufferView& weightView =
                                                    model.bufferViews[weightAccessor.bufferView];
                                                bufferWeights = reinterpret_cast<const float*>(
                                                    &(model.buffers[weightView.buffer]
                                                          .data[weightAccessor.byteOffset + weightView.byteOffset]));
                                                weightByteStride =
                                                    weightAccessor.ByteStride(weightView)
                                                        ? (weightAccessor.ByteStride(weightView) / sizeof(float))
                                                        : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC4);
                                                IC_CORE_INFO("    Found Weight_0 attrib of stride {}",
                                                             weightByteStride);
                                        }

                                        hasSkin = (bufferJoints && bufferWeights);
                                        if (hasSkin)
                                        {
                                                IC_CORE_INFO("Model Contains a skin");
                                        }

                                        for (size_t v = 0; v < posAccessor.count; v++)
                                        {
                                                Vertex& vert = loaderInfo.vertexBuffer[loaderInfo.vertexPos];
                                                vert.pos     = glm::vec4(glm::make_vec3(&bufferPos[v * posByteStride]),
                                                                     1.0f);
                                                vert.normal  = glm::normalize(glm::vec3(
                                                    bufferNormals ? glm::make_vec3(&bufferNormals[v * normByteStride])
                                                                  : glm::vec3(0.0f)));
                                                vert.uv0     = bufferTexCoordSet0
                                                                   ? glm::make_vec2(&bufferTexCoordSet0[v * uv0ByteStride])
                                                                   : glm::vec2(0.0f);
                                                vert.uv1     = bufferTexCoordSet1
                                                                   ? glm::make_vec2(&bufferTexCoordSet1[v * uv1ByteStride])
                                                                   : glm::vec2(0.0f);
                                                vert.color   = bufferColorSet0
                                                                   ? glm::make_vec4(
                                                                       &bufferColorSet0[v * color0ByteStride])
                                                                   : glm::vec4(1.0f);

                                                if (hasSkin)
                                                {
                                                        switch (jointComponentType)
                                                        {
                                                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                                                        {
                                                                const uint16_t* buf = static_cast<const uint16_t*>(
                                                                    bufferJoints);
                                                                vert.joint0 = glm::uvec4(
                                                                    glm::make_vec4(&buf[v * jointByteStride]));
                                                                break;
                                                        }
                                                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                                                        {
                                                                const uint8_t* buf = static_cast<const uint8_t*>(
                                                                    bufferJoints);
                                                                vert.joint0 = glm::vec4(
                                                                    glm::make_vec4(&buf[v * jointByteStride]));
                                                                break;
                                                        }
                                                        default:
                                                                // Not supported by spec
                                                                std::cerr << "Joint component type "
                                                                          << jointComponentType << " not supported!"
                                                                          << std::endl;
                                                                break;
                                                        }
                                                }
                                                else
                                                {
                                                        vert.joint0 = glm::vec4(0.0f);
                                                }
                                                vert.weight0 = hasSkin ? glm::make_vec4(
                                                                             &bufferWeights[v * weightByteStride])
                                                                       : glm::vec4(0.0f);
                                                // Fix for all zero weights
                                                if (glm::length(vert.weight0) == 0.0f)
                                                {
                                                        vert.weight0 = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
                                                }
                                                loaderInfo.vertexPos++;
                                        }
                                }
                                // Indices
                                if (hasIndices)
                                {
                                        const tinygltf::Accessor& accessor =
                                            model.accessors[primitive.indices > -1 ? primitive.indices : 0];
                                        const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                                        const tinygltf::Buffer& buffer         = model.buffers[bufferView.buffer];

                                        indexCount                             = static_cast<uint32_t>(accessor.count);
                                        const void* dataPtr                    = &(
                                            buffer.data[accessor.byteOffset + bufferView.byteOffset]);

                                        switch (accessor.componentType)
                                        {
                                        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
                                        {
                                                const uint32_t* buf = static_cast<const uint32_t*>(dataPtr);
                                                for (size_t index = 0; index < accessor.count; index++)
                                                {
                                                        loaderInfo.indexBuffer[loaderInfo.indexPos] = buf[index] +
                                                                                                      vertexStart;
                                                        loaderInfo.indexPos++;
                                                }
                                                break;
                                        }
                                        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
                                        {
                                                const uint16_t* buf = static_cast<const uint16_t*>(dataPtr);
                                                for (size_t index = 0; index < accessor.count; index++)
                                                {
                                                        loaderInfo.indexBuffer[loaderInfo.indexPos] = buf[index] +
                                                                                                      vertexStart;
                                                        loaderInfo.indexPos++;
                                                }
                                                break;
                                        }
                                        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
                                        {
                                                const uint8_t* buf = static_cast<const uint8_t*>(dataPtr);
                                                for (size_t index = 0; index < accessor.count; index++)
                                                {
                                                        loaderInfo.indexBuffer[loaderInfo.indexPos] = buf[index] +
                                                                                                      vertexStart;
                                                        loaderInfo.indexPos++;
                                                }
                                                break;
                                        }
                                        default:
                                                std::cerr << "Index component type " << accessor.componentType
                                                          << " not supported!" << std::endl;
                                                return;
                                        }
                                }
                                Primitive* newPrimitive   = new Primitive(indexStart,
                                                                        indexCount,
                                                                        vertexCount,
                                                                        primitive.material > -1
                                                                              ? materials[primitive.material]
                                                                              : materials.back());
                                newPrimitive->firstVertex = vertexStart;
                                newPrimitive->vertexCount = vertexCount;
                                newPrimitive->setBoundingBox(posMin, posMax);
                                newMesh->primitives.push_back(newPrimitive);
                        }
                        // Mesh BB from BBs of primitives
                        for (auto p : newMesh->primitives)
                        {
                                if (p->bb.valid && !newMesh->bb.valid)
                                {
                                        newMesh->bb       = p->bb;
                                        newMesh->bb.valid = true;
                                }
                                newMesh->bb.min = glm::min(newMesh->bb.min, p->bb.min);
                                newMesh->bb.max = glm::max(newMesh->bb.max, p->bb.max);
                        }
                        newNode->mesh = newMesh;
                }
                if (parent)
                {
                        parent->children.push_back(newNode);
                }
                else
                {
                        nodes.push_back(newNode);
                }
                linearNodes.push_back(newNode);
        }

        void Model::getNodeProps(const tinygltf::Node& node,
                                 const tinygltf::Model& model,
                                 size_t& vertexCount,
                                 size_t& indexCount)
        {
                if (node.children.size() > 0)
                {
                        for (size_t i = 0; i < node.children.size(); i++)
                        {
                                getNodeProps(model.nodes[node.children[i]], model, vertexCount, indexCount);
                        }
                }
                if (node.mesh > -1)
                {
                        const tinygltf::Mesh mesh = model.meshes[node.mesh];
                        for (size_t i = 0; i < mesh.primitives.size(); i++)
                        {
                                auto& primitive  = mesh.primitives[i];
                                vertexCount     += model.accessors[primitive.attributes.find("POSITION")->second].count;
                                if (primitive.indices > -1)
                                {
                                        indexCount += model.accessors[primitive.indices].count;
                                }
                        }
                }
        }

        void Model::loadImages(tinygltf::Model& gltfModel, ic::vkdevice* device, VkQueue transferQueue) {}

        void Model::loadSkins(tinygltf::Model& gltfModel)
        {
                for (tinygltf::Skin& source : gltfModel.skins)
                {
                        Skin* newSkin = new Skin{};
                        newSkin->name = source.name;

                        // Find skeleton root node
                        if (source.skeleton > -1)
                        {
                                newSkin->skeletonRoot = nodeFromIndex(source.skeleton);
                        }

                        // Find joint nodes
                        for (int jointIndex : source.joints)
                        {
                                Node* node = nodeFromIndex(jointIndex);
                                if (node)
                                {
                                        newSkin->joints.push_back(nodeFromIndex(jointIndex));
                                }
                        }

                        // Get inverse bind matrices from buffer
                        if (source.inverseBindMatrices > -1)
                        {
                                const tinygltf::Accessor& accessor = gltfModel.accessors[source.inverseBindMatrices];
                                const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
                                const tinygltf::Buffer& buffer         = gltfModel.buffers[bufferView.buffer];
                                newSkin->inverseBindMatrices.resize(accessor.count);
                                memcpy(newSkin->inverseBindMatrices.data(),
                                       &buffer.data[accessor.byteOffset + bufferView.byteOffset],
                                       accessor.count * sizeof(glm::mat4));
                        }

                        if (newSkin->joints.size() > MAX_NUM_JOINTS)
                        {
                                std::cerr << "[WARNING] Skin " << newSkin->name << " has " << newSkin->joints.size()
                                          << " joints, which is higher than the supported maximum of " << MAX_NUM_JOINTS
                                          << "\n";
                                std::cerr << "[WARNING] glTF scene may display wrong/incomplete\n";
                        }

                        skins.push_back(newSkin);
                }
        }

        void Model::loadTextures(tinygltf::Model& gltfModel, ic::vkdevice* device, VkQueue transferQueue)
        {
                for (tinygltf::Texture& tex : gltfModel.textures)
                {
                        int source = tex.source;
                        // If this texture uses the KHR_texture_basisu, we need to get the source index from the
                        // extension structure
                        if (tex.extensions.find("KHR_texture_basisu") != tex.extensions.end())
                        {
                                auto ext   = tex.extensions.find("KHR_texture_basisu");
                                auto value = ext->second.Get("source");
                                source     = value.Get<int>();
                        }
                        tinygltf::Image image = gltfModel.images[source];
                        vkLoad::TextureSampler textureSampler;
                        if (tex.sampler == -1)
                        {
                                // No sampler specified, use a default one
                                textureSampler.magFilter    = VK_FILTER_LINEAR;
                                textureSampler.minFilter    = VK_FILTER_LINEAR;
                                textureSampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                                textureSampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                                textureSampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                        }
                        else
                        {
                                textureSampler = textureSamplers[tex.sampler];
                        }
                        vkLoad::Texture texture;
                        texture.fromglTfImage(image, filePath, textureSampler, device, transferQueue);
                        textures.push_back(texture);
                }
        }

        VkSamplerAddressMode Model::getVkWrapMode(int32_t wrapMode)
        {
                switch (wrapMode)
                {
                case -1:
                case 10497:
                        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
                case 33071:
                        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                case 33648:
                        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
                }

                std::cerr << "Unknown wrap mode for getVkWrapMode: " << wrapMode << std::endl;
                return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        }

        VkFilter Model::getVkFilterMode(int32_t filterMode)
        {
                switch (filterMode)
                {
                case -1:
                case 9728:
                        return VK_FILTER_NEAREST;
                case 9729:
                        return VK_FILTER_LINEAR;
                case 9984:
                        return VK_FILTER_NEAREST;
                case 9985:
                        return VK_FILTER_NEAREST;
                case 9986:
                        return VK_FILTER_LINEAR;
                case 9987:
                        return VK_FILTER_LINEAR;
                }

                std::cerr << "Unknown filter mode for getVkFilterMode: " << filterMode << std::endl;
                return VK_FILTER_NEAREST;
        }

        void Model::loadTextureSamplers(tinygltf::Model& gltfModel)
        {
                for (tinygltf::Sampler smpl : gltfModel.samplers)
                {
                        vkLoad::TextureSampler sampler{};
                        sampler.minFilter    = getVkFilterMode(smpl.minFilter);
                        sampler.magFilter    = getVkFilterMode(smpl.magFilter);
                        sampler.addressModeU = getVkWrapMode(smpl.wrapS);
                        sampler.addressModeV = getVkWrapMode(smpl.wrapT);
                        sampler.addressModeW = sampler.addressModeV;
                        textureSamplers.push_back(sampler);
                }
                IC_CORE_INFO("Texture Sampler Count: {}", textureSamplers.size());
        }

        void Model::loadMaterials(tinygltf::Model& gltfModel)
        {
                IC_CORE_INFO("Loading Materials!");
                for (tinygltf::Material& mat : gltfModel.materials)
                {
                        IC_CORE_INFO("Loading material {}", mat.name);
                        vkLoad::Material material(device);
                        material.doubleSided = mat.doubleSided;
                        if (mat.values.find("baseColorTexture") != mat.values.end())
                        {
                                IC_CORE_INFO("    Found baseColorTexture attrib");
                                material.baseColorTexture = &textures[mat.values["baseColorTexture"].TextureIndex()];
                                material.texCoordSets.baseColor = mat.values["baseColorTexture"].TextureTexCoord();
                        }
                        if (mat.values.find("metallicRoughnessTexture") != mat.values.end())
                        {
                                IC_CORE_INFO("    Found metallicRoughnessTexture attrib");
                                material.metallicRoughnessTexture =
                                    &textures[mat.values["metallicRoughnessTexture"].TextureIndex()];
                                material.texCoordSets.metallicRoughness =
                                    mat.values["metallicRoughnessTexture"].TextureTexCoord();
                        }
                        if (mat.values.find("roughnessFactor") != mat.values.end())
                        {
                                IC_CORE_INFO("    Found roughnessFactor attrib");
                                material.roughnessFactor = static_cast<float>(mat.values["roughnessFactor"].Factor());
                        }
                        if (mat.values.find("metallicFactor") != mat.values.end())
                        {
                                IC_CORE_INFO("    Found metallicFactor attrib");
                                material.metallicFactor = static_cast<float>(mat.values["metallicFactor"].Factor());
                        }
                        if (mat.values.find("baseColorFactor") != mat.values.end())
                        {
                                IC_CORE_INFO("    Found baseColorFactor attrib");
                                material.baseColorFactor = glm::make_vec4(
                                    mat.values["baseColorFactor"].ColorFactor().data());
                        }
                        if (mat.additionalValues.find("normalTexture") != mat.additionalValues.end())
                        {
                                IC_CORE_INFO("    Found normalTexture attrib");
                                material.normalTexture = &textures[mat.additionalValues["normalTexture"].TextureIndex()];
                                material.texCoordSets.normal = mat.additionalValues["normalTexture"].TextureTexCoord();
                        }
                        if (mat.additionalValues.find("emissiveTexture") != mat.additionalValues.end())
                        {
                                IC_CORE_INFO("    Found emissiveTexture attrib");
                                material.emissiveTexture =
                                    &textures[mat.additionalValues["emissiveTexture"].TextureIndex()];
                                material.texCoordSets.emissive =
                                    mat.additionalValues["emissiveTexture"].TextureTexCoord();
                        }
                        if (mat.additionalValues.find("occlusionTexture") != mat.additionalValues.end())
                        {
                                IC_CORE_INFO("    Found occlusionTexture attrib");
                                material.occlusionTexture =
                                    &textures[mat.additionalValues["occlusionTexture"].TextureIndex()];
                                material.texCoordSets.occlusion =
                                    mat.additionalValues["occlusionTexture"].TextureTexCoord();
                        }
                        if (mat.additionalValues.find("alphaMode") != mat.additionalValues.end())
                        {
                                IC_CORE_INFO("    Found alphaMode attrib");
                                tinygltf::Parameter param = mat.additionalValues["alphaMode"];
                                if (param.string_value == "BLEND")
                                {
                                        material.alphaMode = Material::ALPHAMODE_BLEND;
                                }
                                if (param.string_value == "MASK")
                                {
                                        material.alphaCutoff = 0.5f;
                                        material.alphaMode   = Material::ALPHAMODE_MASK;
                                }
                        }
                        if (mat.additionalValues.find("alphaCutoff") != mat.additionalValues.end())
                        {
                                IC_CORE_INFO("    Found alphaCutoff attrib");
                                material.alphaCutoff = static_cast<float>(mat.additionalValues["alphaCutoff"].Factor());
                        }
                        if (mat.additionalValues.find("emissiveFactor") != mat.additionalValues.end())
                        {
                                IC_CORE_INFO("    Found emissiveFactor attrib");
                                material.emissiveFactor = glm::vec4(
                                    glm::make_vec3(mat.additionalValues["emissiveFactor"].ColorFactor().data()), 1.0);
                        }

                        // Extensions
                        if (mat.extensions.find("KHR_materials_pbrSpecularGlossiness") != mat.extensions.end())
                        {
                                IC_CORE_INFO("    Found pbrSpecularGlossiness extension");
                                auto ext = mat.extensions.find("KHR_materials_pbrSpecularGlossiness");
                                if (ext->second.Has("specularGlossinessTexture"))
                                {
                                        auto index = ext->second.Get("specularGlossinessTexture").Get("index");
                                        material.extension.specularGlossinessTexture = &textures[index.Get<int>()];
                                        auto texCoordSet = ext->second.Get("specularGlossinessTexture").Get("texCoord");
                                        material.texCoordSets.specularGlossiness = texCoordSet.Get<int>();
                                        material.pbrWorkflows.specularGlossiness = true;
                                        material.pbrWorkflows.metallicRoughness  = false;
                                }
                                if (ext->second.Has("diffuseTexture"))
                                {
                                        auto index = ext->second.Get("diffuseTexture").Get("index");
                                        material.extension.diffuseTexture = &textures[index.Get<int>()];
                                }
                                if (ext->second.Has("diffuseFactor"))
                                {
                                        auto factor = ext->second.Get("diffuseFactor");
                                        for (uint32_t i = 0; i < factor.ArrayLen(); i++)
                                        {
                                                auto val                            = factor.Get(i);
                                                material.extension.diffuseFactor[i] = val.IsNumber()
                                                                                          ? (float)val.Get<double>()
                                                                                          : (float)val.Get<int>();
                                        }
                                }
                                if (ext->second.Has("specularFactor"))
                                {
                                        auto factor = ext->second.Get("specularFactor");
                                        for (uint32_t i = 0; i < factor.ArrayLen(); i++)
                                        {
                                                auto val                             = factor.Get(i);
                                                material.extension.specularFactor[i] = val.IsNumber()
                                                                                           ? (float)val.Get<double>()
                                                                                           : (float)val.Get<int>();
                                        }
                                }
                        }

                        if (mat.extensions.find("KHR_materials_unlit") != mat.extensions.end())
                        {
                                material.unlit = true;
                        }

                        if (mat.extensions.find("KHR_materials_emissive_strength") != mat.extensions.end())
                        {
                                auto ext = mat.extensions.find("KHR_materials_emissive_strength");
                                if (ext->second.Has("emissiveStrength"))
                                {
                                        auto value                = ext->second.Get("emissiveStrength");
                                        material.emissiveStrength = (float)value.Get<double>();
                                }
                        }

                        material.index = static_cast<uint32_t>(materials.size());
                        materials.push_back(material);
                }
                // Push a default material at the end of the list for meshes with no material assigned
                materials.push_back(Material(device));

                IC_CORE_INFO("Loaded {} Materials!", materials.size());
        }

        void Model::loadAnimations(tinygltf::Model& gltfModel)
        {
                for (tinygltf::Animation& anim : gltfModel.animations)
                {
                        vkLoad::Animation animation{};
                        animation.name = anim.name;
                        if (anim.name.empty())
                        {
                                animation.name = std::to_string(animations.size());
                        }

                        // Samplers
                        for (auto& samp : anim.samplers)
                        {
                                vkLoad::AnimationSampler sampler{};

                                if (samp.interpolation == "LINEAR")
                                {
                                        sampler.interpolation = AnimationSampler::InterpolationType::LINEAR;
                                }
                                if (samp.interpolation == "STEP")
                                {
                                        sampler.interpolation = AnimationSampler::InterpolationType::STEP;
                                }
                                if (samp.interpolation == "CUBICSPLINE")
                                {
                                        sampler.interpolation = AnimationSampler::InterpolationType::CUBICSPLINE;
                                }

                                // Read sampler input time values
                                {
                                        const tinygltf::Accessor& accessor = gltfModel.accessors[samp.input];
                                        const tinygltf::BufferView& bufferView =
                                            gltfModel.bufferViews[accessor.bufferView];
                                        const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

                                        assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

                                        const void* dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
                                        const float* buf    = static_cast<const float*>(dataPtr);
                                        for (size_t index = 0; index < accessor.count; index++)
                                        {
                                                sampler.inputs.push_back(buf[index]);
                                        }

                                        for (auto input : sampler.inputs)
                                        {
                                                if (input < animation.start)
                                                {
                                                        animation.start = input;
                                                };
                                                if (input > animation.end)
                                                {
                                                        animation.end = input;
                                                }
                                        }
                                }

                                // Read sampler output T/R/S values
                                {
                                        const tinygltf::Accessor& accessor = gltfModel.accessors[samp.output];
                                        const tinygltf::BufferView& bufferView =
                                            gltfModel.bufferViews[accessor.bufferView];
                                        const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

                                        assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

                                        const void* dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];

                                        switch (accessor.type)
                                        {
                                        case TINYGLTF_TYPE_VEC3:
                                        {
                                                const glm::vec3* buf = static_cast<const glm::vec3*>(dataPtr);
                                                for (size_t index = 0; index < accessor.count; index++)
                                                {
                                                        sampler.outputsVec4.push_back(glm::vec4(buf[index], 0.0f));
                                                        sampler.outputs.push_back(buf[index][0]);
                                                        sampler.outputs.push_back(buf[index][1]);
                                                        sampler.outputs.push_back(buf[index][2]);
                                                }
                                                break;
                                        }
                                        case TINYGLTF_TYPE_VEC4:
                                        {
                                                const glm::vec4* buf = static_cast<const glm::vec4*>(dataPtr);
                                                for (size_t index = 0; index < accessor.count; index++)
                                                {
                                                        sampler.outputsVec4.push_back(buf[index]);
                                                        sampler.outputs.push_back(buf[index][0]);
                                                        sampler.outputs.push_back(buf[index][1]);
                                                        sampler.outputs.push_back(buf[index][2]);
                                                        sampler.outputs.push_back(buf[index][3]);
                                                }
                                                break;
                                        }
                                        default:
                                        {
                                                std::cout << "unknown type" << std::endl;
                                                break;
                                        }
                                        }
                                }

                                animation.samplers.push_back(sampler);
                        }

                        // Channels
                        for (auto& source : anim.channels)
                        {
                                vkLoad::AnimationChannel channel{};

                                if (source.target_path == "rotation")
                                {
                                        channel.path = AnimationChannel::PathType::ROTATION;
                                }
                                if (source.target_path == "translation")
                                {
                                        channel.path = AnimationChannel::PathType::TRANSLATION;
                                }
                                if (source.target_path == "scale")
                                {
                                        channel.path = AnimationChannel::PathType::SCALE;
                                }
                                if (source.target_path == "weights")
                                {
                                        std::cout << "weights not yet supported, skipping channel" << std::endl;
                                        continue;
                                }
                                channel.samplerIndex = source.sampler;
                                channel.node         = nodeFromIndex(source.target_node);
                                if (!channel.node)
                                {
                                        continue;
                                }

                                animation.channels.push_back(channel);
                        }

                        animations.push_back(animation);
                }
        }

        void Model::loadFromFile(
            std::string filename, ic::vkdevice* device, VkQueue transferQueue, uint32_t fileLoadingFlags, float scale)
        {
                tinygltf::Model gltfModel;
                tinygltf::TinyGLTF gltfContext;

                // @todo
                if (fileLoadingFlags & FileLoadingFlags::DontLoadImages)
                {
                        gltfContext.SetImageLoader(loadImageDataFuncEmpty, nullptr);
                }
                else
                {
                        gltfContext.SetImageLoader(loadImageDataFunc, nullptr);
                }

                std::string error, warning;

                this->device  = device;

                bool binary   = false;
                size_t extpos = filename.rfind('.', filename.length());
                if (extpos != std::string::npos)
                {
                        binary = (filename.substr(extpos + 1, filename.length() - extpos) == "glb");
                }

                size_t pos = filename.find_last_of('/');
                if (pos == std::string::npos)
                {
                        pos = filename.find_last_of('\\');
                }
                filePath = filename.substr(0, pos);

                IC_CORE_INFO("FilePath to Model:{}", filePath);

                bool fileLoaded = false;
                if (binary)
                {
                        IC_CORE_INFO("Loading binary data");
                        fileLoaded = gltfContext.LoadBinaryFromFile(&gltfModel, &error, &warning, filename.c_str());
                }
                else
                {
                        IC_CORE_INFO("Loading ASCII data");
                        fileLoaded = gltfContext.LoadASCIIFromFile(&gltfModel, &error, &warning, filename.c_str());
                }
                IC_CORE_INFO("Buffer Size: {}", gltfModel.buffers.size());

                LoaderInfo loaderInfo{};
                size_t vertexCount = 0;
                size_t indexCount  = 0;

                if (fileLoaded)
                {
                        if (!(fileLoadingFlags & FileLoadingFlags::DontLoadImages))
                        {
                                loadImages(gltfModel, device, transferQueue);
                        }
                        extensions = gltfModel.extensionsUsed;
                        for (auto& extension : extensions)
                        {
                                // If this model uses basis universal compressed textures, we need to transcode
                                // them So we need to initialize that transcoder once
                                if (extension == "KHR_texture_basisu")
                                {
                                        std::cout << "Model uses KHR_texture_basisu, initializing basisu "
                                                     "transcoder\n";
                                        basist::basisu_transcoder_init();
                                }
                        }

                        loadTextureSamplers(gltfModel);
                        loadTextures(gltfModel, device, transferQueue);
                        loadMaterials(gltfModel);

                        const tinygltf::Scene& scene =
                            gltfModel.scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];

                        // Get vertex and index buffer sizes up-front
                        for (size_t i = 0; i < scene.nodes.size(); i++)
                        {
                                getNodeProps(gltfModel.nodes[scene.nodes[i]], gltfModel, vertexCount, indexCount);
                        }

                        IC_CORE_INFO("Total Vertex Count: {}", vertexCount);
                        IC_CORE_INFO("Total Index Count: {}", indexCount);
                        IC_CORE_INFO("Loading {} Nodes!", scene.nodes.size());

                        loaderInfo.vertexBuffer = new Vertex[vertexCount];
                        loaderInfo.indexBuffer  = new uint32_t[indexCount];

                        // TODO: scene handling with no default scene
                        for (size_t i = 0; i < scene.nodes.size(); i++)
                        {
                                const tinygltf::Node node = gltfModel.nodes[scene.nodes[i]];
                                IC_CORE_INFO("Loading Node: {}", node.name);
                                loadNode(nullptr, node, scene.nodes[i], gltfModel, loaderInfo, scale);
                        }
                        if (gltfModel.animations.size() > 0)
                        {
                                loadAnimations(gltfModel);
                        }
                        loadSkins(gltfModel);

                        uint32_t meshIndex = 0;
                        for (auto node : linearNodes)
                        {
                                // Assign skins
                                if (node->skinIndex > -1)
                                {
                                        node->skin = skins[node->skinIndex];
                                }
                                // Initial pose
                                if (node->mesh)
                                {
                                        node->mesh->index = meshIndex++;
                                        node->update();
                                }
                        }
                }
                else
                {
                        // TODO: throw
                        IC_CORE_CRITICAL("Could not load gltf file: {}", error);
                        return;
                }

                // Pre-Calculations for requested features
                if ((fileLoadingFlags & FileLoadingFlags::PreTransformVertices) ||
                    (fileLoadingFlags & FileLoadingFlags::PreMultiplyVertexColors) ||
                    (fileLoadingFlags & FileLoadingFlags::FlipY))
                {
                        const bool preTransform     = fileLoadingFlags & FileLoadingFlags::PreTransformVertices;
                        const bool preMultiplyColor = fileLoadingFlags & FileLoadingFlags::PreMultiplyVertexColors;
                        const bool flipY            = fileLoadingFlags & FileLoadingFlags::FlipY;
                        for (Node* node : linearNodes)
                        {
                                if (node->mesh)
                                {
                                        const glm::mat4 localMatrix = node->getMatrix();
                                        for (Primitive* primitive : node->mesh->primitives)
                                        {
                                                for (uint32_t i = 0; i < primitive->vertexCount; i++)
                                                {
                                                        Vertex& vertex =
                                                            loaderInfo.vertexBuffer[primitive->firstVertex + i];
                                                        // Pre-transform vertex positions by node-hierarchy
                                                        if (preTransform)
                                                        {
                                                                vertex.pos    = glm::vec3(localMatrix *
                                                                                       glm::vec4(vertex.pos, 1.0f));
                                                                vertex.normal = glm::normalize(glm::mat3(localMatrix) *
                                                                                               vertex.normal);
                                                        }
                                                        // Flip Y-Axis of vertex positions
                                                        if (flipY)
                                                        {
                                                                vertex.pos.y    *= -1.0f;
                                                                vertex.normal.y *= -1.0f;
                                                        }
                                                        // Pre-Multiply vertex colors with material base color
                                                        if (preMultiplyColor)
                                                        {
                                                                vertex.color = primitive->material.baseColorFactor *
                                                                               vertex.color;
                                                        }
                                                }
                                        }
                                }
                        }
                }

                size_t vertexBufferSize = vertexCount * sizeof(Vertex);
                size_t indexBufferSize  = indexCount * sizeof(uint32_t);

                assert(vertexBufferSize > 0);

                struct StagingBuffer
                {
                        VkBuffer buffer;
                        VkDeviceMemory memory;
                } vertexStaging, indexStaging;

                // Create staging buffers
                // Vertex data
                IC_CORE_ASSERT(device->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                                    vertexBufferSize,
                                                    &vertexStaging.buffer,
                                                    &vertexStaging.memory,
                                                    loaderInfo.vertexBuffer) == VK_SUCCESS,
                               "Failed to create buffer from file!");
                // Index data
                if (indexBufferSize > 0)
                {
                        IC_CORE_ASSERT(device->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                                            indexBufferSize,
                                                            &indexStaging.buffer,
                                                            &indexStaging.memory,
                                                            loaderInfo.indexBuffer) == VK_SUCCESS,
                                       "Failed to create index buffer for the given file");
                }

                // Create device local buffers
                // Vertex buffer
                IC_CORE_ASSERT(device->createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                                        VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                    vertexBufferSize,
                                                    &vertices.buffer,
                                                    &vertices.memory) == VK_SUCCESS,
                               "Failed to create vertex buffer for the file");
                // Index buffer
                if (indexBufferSize > 0)
                {
                        IC_CORE_ASSERT(device->createBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                                                VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                            indexBufferSize,
                                                            &indices.buffer,
                                                            &indices.memory) == VK_SUCCESS,
                                       "Failed to create index buffer for the file");
                }

                // Copy from staging buffers
                VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

                VkBufferCopy copyRegion = {};

                copyRegion.size         = vertexBufferSize;
                vkCmdCopyBuffer(copyCmd, vertexStaging.buffer, vertices.buffer, 1, &copyRegion);

                if (indexBufferSize > 0)
                {
                        copyRegion.size = indexBufferSize;
                        vkCmdCopyBuffer(copyCmd, indexStaging.buffer, indices.buffer, 1, &copyRegion);
                }

                device->flushCommandBuffer(copyCmd, transferQueue, true);

                vkDestroyBuffer(device->logicalDevice, vertexStaging.buffer, nullptr);
                vkFreeMemory(device->logicalDevice, vertexStaging.memory, nullptr);
                if (indexBufferSize > 0)
                {
                        vkDestroyBuffer(device->logicalDevice, indexStaging.buffer, nullptr);
                        vkFreeMemory(device->logicalDevice, indexStaging.memory, nullptr);
                }

                delete[] loaderInfo.vertexBuffer;
                delete[] loaderInfo.indexBuffer;

                getSceneDimensions();

                // Setup descriptors
                uint32_t uboCount{0};
                uint32_t imageCount{0};
                for (auto& node : linearNodes)
                {
                        if (node->mesh)
                        {
                                uboCount++;
                        }
                }
                for (auto& material : materials)
                {
                        // Only check texture properties if the material has a texture
                        if (material.baseColorTexture)
                        {
                                IC_CORE_ASSERT(material.baseColorTexture->descriptor.imageView != VK_NULL_HANDLE,
                                               "Imageview was corrupt");
                                IC_CORE_ASSERT(material.baseColorTexture->descriptor.imageLayout ==
                                                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                               "Layout not set properly");
                        }
                        if (material.baseColorTexture != nullptr)
                        {
                                imageCount++;
                        }
                }

                IC_CORE_INFO("Creating descriptor Pools for {} UBOs and {} materials", uboCount, imageCount);
                std::vector<VkDescriptorPoolSize> poolSizes = {
                    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uboCount},
                };

                // TODO put this as a changeable property.
                uint32_t descriptorBindingFlags = vkLoad::DescriptorBindingFlags::ImageBaseColor;
                if (imageCount > 0)
                {
                        if (descriptorBindingFlags & DescriptorBindingFlags::ImageBaseColor)
                        {
                                poolSizes.push_back({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageCount});
                        }
                        if (descriptorBindingFlags & DescriptorBindingFlags::ImageNormalMap)
                        {
                                poolSizes.push_back({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageCount});
                        }
                }

                VkDescriptorPoolCreateInfo descriptorPoolCI{.sType   = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                                                            .maxSets = uboCount + imageCount,
                                                            .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
                                                            .pPoolSizes    = poolSizes.data()};
                IC_CORE_ASSERT(vkCreateDescriptorPool(
                                   device->logicalDevice, &descriptorPoolCI, nullptr, &descriptorPool) == VK_SUCCESS,
                               "Failed to create descriptor Pool");

                // Descriptors for per-node uniform buffers
                {
                        // Layout is global, so only create if it hasn't already been created before
                        if (descriptorSetLayoutUbo == VK_NULL_HANDLE)
                        {
                                IC_CORE_INFO("Creating Descriptor Layout for UBO");
                                VkDescriptorSetLayoutBinding setLayoutBinding{.binding = 0,
                                                                              .descriptorType =
                                                                                  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                                              .descriptorCount = 1,
                                                                              .stageFlags = VK_SHADER_STAGE_VERTEX_BIT};
                                VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{
                                    .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                                    .bindingCount = 1,
                                    .pBindings    = &setLayoutBinding};
                                IC_CORE_ASSERT(vkCreateDescriptorSetLayout(device->logicalDevice,
                                                                           &descriptorLayoutCI,
                                                                           nullptr,
                                                                           &descriptorSetLayoutUbo) == VK_SUCCESS,
                                               "Failed to create Descriptor Layouts");
                        }
                        for (auto node : nodes)
                        {
                                prepareNodeDescriptor(node, descriptorSetLayoutUbo);
                        }
                }

                // Descriptors for per-material images
                {
                        // Layout is global, so only create if it hasn't already been created before
                        if (descriptorSetLayoutImage == VK_NULL_HANDLE)
                        {
                                std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
                                if (descriptorBindingFlags & DescriptorBindingFlags::ImageBaseColor)
                                {
                                        setLayoutBindings.push_back(
                                            {.binding         = 1,
                                             .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                             .descriptorCount = 1,
                                             .stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT});
                                }
                                if (descriptorBindingFlags & DescriptorBindingFlags::ImageNormalMap)
                                {
                                        setLayoutBindings.push_back(
                                            {.binding         = 1,
                                             .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                             .descriptorCount = 1,
                                             .stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT});
                                }
                                VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{
                                    .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                                    .bindingCount = static_cast<uint32_t>(setLayoutBindings.size()),
                                    .pBindings    = setLayoutBindings.data(),
                                };
                                IC_CORE_ASSERT(vkCreateDescriptorSetLayout(device->logicalDevice,
                                                                           &descriptorLayoutCI,
                                                                           nullptr,
                                                                           &descriptorSetLayoutImage) == VK_SUCCESS,
                                               "Failed to create Descriptr sets layout for images.");
                        }
                        for (auto& material : materials)
                        {
                                if (material.baseColorTexture != nullptr)
                                {
                                        material.createDescriptorSet(descriptorPool,
                                                                     vkLoad::descriptorSetLayoutImage,
                                                                     descriptorBindingFlags);
                                }
                        }
                }
        }

        void Model::drawNode(Node* node, VkCommandBuffer commandBuffer)
        {
                if (node->mesh)
                {
                        for (Primitive* primitive : node->mesh->primitives)
                        {
                                vkCmdDrawIndexed(commandBuffer,
                                                 primitive->indexCount,
                                                 1,
                                                 primitive->firstIndex,
                                                 primitive->firstVertex,
                                                 0);
                        }
                }
                for (auto& child : node->children)
                {
                        drawNode(child, commandBuffer);
                }
        }

        void Model::draw(VkCommandBuffer commandBuffer)
        {
                const VkDeviceSize offsets[1] = {0};
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices.buffer, offsets);
                vkCmdBindIndexBuffer(commandBuffer, indices.buffer, 0, VK_INDEX_TYPE_UINT32);
                for (auto& node : nodes)
                {
                        drawNode(node, commandBuffer);
                }
        }

        void Model::calculateBoundingBox(Node* node, Node* parent)
        {
                BoundingBox parentBvh = parent ? parent->bvh : BoundingBox(dimensions.min, dimensions.max);

                if (node->mesh)
                {
                        if (node->mesh->bb.valid)
                        {
                                node->aabb = node->mesh->bb.getAABB(node->getMatrix());
                                if (node->children.size() == 0)
                                {
                                        node->bvh.min   = node->aabb.min;
                                        node->bvh.max   = node->aabb.max;
                                        node->bvh.valid = true;
                                }
                        }
                }

                parentBvh.min = glm::min(parentBvh.min, node->bvh.min);
                parentBvh.max = glm::min(parentBvh.max, node->bvh.max);

                for (auto& child : node->children)
                {
                        calculateBoundingBox(child, node);
                }
        }

        void Model::getSceneDimensions()
        {
                // Calculate binary volume hierarchy for all nodes in the scene
                for (auto node : linearNodes)
                {
                        calculateBoundingBox(node, nullptr);
                }

                dimensions.min = glm::vec3(FLT_MAX);
                dimensions.max = glm::vec3(-FLT_MAX);

                for (auto node : linearNodes)
                {
                        if (node->bvh.valid)
                        {
                                dimensions.min = glm::min(dimensions.min, node->bvh.min);
                                dimensions.max = glm::max(dimensions.max, node->bvh.max);
                        }
                }

                // Calculate scene aabb
                aabb       = glm::scale(glm::mat4(1.0f),
                                  glm::vec3(dimensions.max[0] - dimensions.min[0],
                                            dimensions.max[1] - dimensions.min[1],
                                            dimensions.max[2] - dimensions.min[2]));
                aabb[3][0] = dimensions.min[0];
                aabb[3][1] = dimensions.min[1];
                aabb[3][2] = dimensions.min[2];
        }

        void Model::updateAnimation(uint32_t index, float time)
        {
                if (animations.empty())
                {
                        std::cout << ".glTF does not contain animation." << std::endl;
                        return;
                }
                if (index > static_cast<uint32_t>(animations.size()) - 1)
                {
                        std::cout << "No animation with index " << index << std::endl;
                        return;
                }
                Animation& animation = animations[index];

                bool updated         = false;
                for (auto& channel : animation.channels)
                {
                        vkLoad::AnimationSampler& sampler = animation.samplers[channel.samplerIndex];
                        if (sampler.inputs.size() > sampler.outputsVec4.size())
                        {
                                continue;
                        }

                        for (size_t i = 0; i < sampler.inputs.size() - 1; i++)
                        {
                                if ((time >= sampler.inputs[i]) && (time <= sampler.inputs[i + 1]))
                                {
                                        float u = std::max(0.0f, time - sampler.inputs[i]) /
                                                  (sampler.inputs[i + 1] - sampler.inputs[i]);
                                        if (u <= 1.0f)
                                        {
                                                switch (channel.path)
                                                {
                                                case vkLoad::AnimationChannel::PathType::TRANSLATION:
                                                        sampler.translate(i, time, channel.node);
                                                        break;
                                                case vkLoad::AnimationChannel::PathType::SCALE:
                                                        sampler.scale(i, time, channel.node);
                                                        break;
                                                case vkLoad::AnimationChannel::PathType::ROTATION:
                                                        sampler.rotate(i, time, channel.node);
                                                        break;
                                                }
                                                updated = true;
                                        }
                                }
                        }
                }
                if (updated)
                {
                        for (auto& node : nodes)
                        {
                                node->update();
                        }
                }
        }

        Node* Model::findNode(Node* parent, uint32_t index)
        {
                Node* nodeFound = nullptr;
                if (parent->index == index)
                {
                        return parent;
                }
                for (auto& child : parent->children)
                {
                        nodeFound = findNode(child, index);
                        if (nodeFound)
                        {
                                break;
                        }
                }
                return nodeFound;
        }

        Node* Model::nodeFromIndex(uint32_t index)
        {
                Node* nodeFound = nullptr;
                for (auto& node : nodes)
                {
                        nodeFound = findNode(node, index);
                        if (nodeFound)
                        {
                                break;
                        }
                }
                return nodeFound;
        }

        void Model::prepareNodeDescriptor(Node* node, VkDescriptorSetLayout descriptorSetLayout)
        {
                if (node->mesh)
                {
                        VkDescriptorSetAllocateInfo descriptorSetAllocInfo{
                            .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                            .descriptorPool     = descriptorPool,
                            .descriptorSetCount = 1,
                            .pSetLayouts        = &descriptorSetLayout};
                        IC_CORE_ASSERT(vkAllocateDescriptorSets(device->logicalDevice,
                                                                &descriptorSetAllocInfo,
                                                                &node->mesh->uniformBuffer.descriptorSet) == VK_SUCCESS,
                                       "Failed to allocate descriptors!");
                        VkWriteDescriptorSet writeDescriptorSet{.sType      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                                                .dstSet     = node->mesh->uniformBuffer.descriptorSet,
                                                                .dstBinding = 0,
                                                                .descriptorCount = 1,
                                                                .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                                .pBufferInfo = &node->mesh->uniformBuffer.descriptor};
                        vkUpdateDescriptorSets(device->logicalDevice, 1, &writeDescriptorSet, 0, nullptr);
                }
                for (auto& child : node->children)
                {
                        prepareNodeDescriptor(child, descriptorSetLayout);
                }
        }
}  // namespace vkLoad
