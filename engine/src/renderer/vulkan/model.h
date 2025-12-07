#pragma once

#include "defines.h"
#include "device.h"
#include "texture.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <basisu_transcoder.h>

// [TODO IMP]: This is just a testing branch and hence I am for now going to link with tinygltf as it is simpler
// However later I am goint to probably use fastgltf as it is the modern thing to use after reading the new vulkan 1.4
// docs I am right now just following the sascha willems vulkan example guide

// Define these classes and structs away from each other and put these in an API
// [TODO]: Make API for this like renderer
#define MAX_NUM_JOINTS 128u

namespace vkLoad
{
        struct Node;

        /** @brief A bounding box is a cuboid about a shape. This cuboid can also occur in differenet cases. OBB and
         * AABB An oriented bounding box follows the objects axis and ignores the outside world while an axis aligned
         * bouding box chose to go with the axis of world coordinates and ignores the object coordinates. These are
         * usually given my orientation matrix with a transform that simplifies the min and max along an axis. [TODO]
         * Add lines around bounding boxes and toggle to see them
         */
        struct BoundingBox
        {
                glm::vec3 min;
                glm::vec3 max;
                bool valid = false;
                BoundingBox();
                BoundingBox(glm::vec3 min, glm::vec3 max);
                BoundingBox getAABB(glm::mat4 transformMatrix);
        };

        /** @brief A material is any primitive surface texture that tries to blend with real life images and bodies. */
        struct Material
        {
                /** @brief Alphamode is the amount of transparency for the texture */
                ic::vkdevice* device;
                enum AlphaMode
                {
                        ALPHAMODE_OPAQUE,
                        ALPHAMODE_MASK,
                        ALPHAMODE_BLEND
                };
                AlphaMode alphaMode                       = ALPHAMODE_OPAQUE;
                float alphaCutoff                         = 1.0f;
                float metallicFactor                      = 1.0f;
                float roughnessFactor                     = 1.0f;

                glm::vec4 baseColorFactor                 = glm::vec4(1.0f);
                glm::vec4 emissiveFactor                  = glm::vec4(0.0f);
                vkLoad::Texture* baseColorTexture         = nullptr;
                vkLoad::Texture* metallicRoughnessTexture = nullptr;
                vkLoad::Texture* normalTexture            = nullptr;
                vkLoad::Texture* occlusionTexture         = nullptr;
                vkLoad::Texture* emissiveTexture          = nullptr;

                enum DescriptorBindingFlags
                {
                        ImageBaseColor = 0x00000001,
                        ImageNormalMap = 0x00000002
                };

                bool doubleSided = false;
                struct TexCoordSets
                {
                        uint8_t baseColor          = 0;
                        uint8_t metallicRoughness  = 0;
                        uint8_t specularGlossiness = 0;
                        uint8_t normal             = 0;
                        uint8_t occlusion          = 0;
                        uint8_t emissive           = 0;
                } texCoordSets;

                struct Extension
                {
                        vkLoad::Texture* specularGlossinessTexture = nullptr;
                        vkLoad::Texture* diffuseTexture            = nullptr;
                        glm::vec4 diffuseFactor                    = glm::vec4(1.0f);
                        glm::vec3 specularFactor                   = glm::vec3(0.0f);
                } extension;

                struct PbrWorkflows
                {
                        bool metallicRoughness  = true;
                        bool specularGlossiness = false;
                } pbrWorkflows;

                VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

                int index                     = 0;
                bool unlit                    = false;
                float emissiveStrength        = 1.0f;

                Material(ic::vkdevice* device) : device(device) {}
                void createDescriptorSet(VkDescriptorPool descriptorPool,
                                         VkDescriptorSetLayout setLayout,
                                         uint32_t descriptorBindingFlags);
        };

        /** @brief A primitive is a group of vertex and indices that make up a mesh. It can be with or without material
         */
        struct Primitive
        {
                uint32_t firstIndex;
                uint32_t indexCount;
                uint32_t firstVertex;
                uint32_t vertexCount;
                Material& material;
                bool hasIndices;
                BoundingBox bb;
                Primitive(uint32_t firstIndex, uint32_t indexCount, uint32_t vertexCount, Material& material)
                    : firstIndex(firstIndex), indexCount(indexCount), vertexCount(vertexCount), material(material)
                {
                }
                void setBoundingBox(glm::vec3 min, glm::vec3 max);
        };

        /** @brief A mesh is a group of primitives making up an object with materials and transformations */
        struct Mesh
        {
                ic::vkdevice* device;

                std::vector<Primitive*> primitives;
                std::string name;

                BoundingBox bb;
                BoundingBox aabb;

                struct UniformBuffer
                {
                        VkBuffer buffer;
                        VkDeviceMemory memory;
                        VkDescriptorBufferInfo descriptor;
                        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
                        void* mapped;
                } uniformBuffer;

                struct UniformBlock
                {
                        glm::mat4 matrix;
                        glm::mat4 jointMatrix[MAX_NUM_JOINTS]{};
                        uint32_t jointcount{0};
                } uniformBlock;

                uint32_t index;
                Mesh(ic::vkdevice* device, glm::mat4 matrix);
                ~Mesh();
                void setBoundingBox(glm::vec3 min, glm::vec3 max);
        };

        /** @brief A skin is just a bone structure for a gltf model. It tells how to connect multiple parts of a body to
         * move them coherently with an animation */
        struct Skin
        {
                std::string name;
                Node* skeletonRoot = nullptr;
                std::vector<glm::mat4> inverseBindMatrices;
                std::vector<Node*> joints;
        };

        /** @brief A node is a scene hierarchy object. A node having zero or one parents have children and index with
         * matrices for transformation. A can contain meshes with materials, skins etc. They also have a names. A node
         * is just a representation of hierarchy of data and nothing else in glTF*/
        struct Node
        {
                Node* parent;
                uint32_t index;
                std::vector<Node*> children;

                glm::mat4 matrix;
                std::string name;
                Mesh* mesh;
                Skin* skin;
                int32_t skinIndex = -1;

                glm::vec3 translation{};
                glm::vec3 scale{1.0f};
                glm::quat rotation{};

                BoundingBox bvh;
                BoundingBox aabb;

                bool useCachedMatrix{false};
                glm::mat4 cachedLocalMatrix{glm::mat4(1.0f)};
                glm::mat4 cachedMatrix{glm::mat4(1.0f)};

                glm::mat4 localMatrix();
                glm::mat4 getMatrix();
                void update();
                ~Node();
        };

        struct AnimationChannel
        {
                enum PathType
                {
                        TRANSLATION,
                        ROTATION,
                        SCALE
                };
                PathType path;
                Node* node;
                uint32_t samplerIndex;
        };

        struct AnimationSampler
        {
                enum InterpolationType
                {
                        LINEAR,
                        STEP,
                        CUBICSPLINE
                };
                InterpolationType interpolation;
                std::vector<float> inputs;
                std::vector<glm::vec4> outputsVec4;
                std::vector<float> outputs;
                glm::vec4 cubicSplineInterpolation(size_t index, float time, uint32_t stride);
                void translate(size_t index, float time, vkLoad::Node* node);
                void scale(size_t index, float time, vkLoad::Node* node);
                void rotate(size_t index, float time, vkLoad::Node* node);
        };

        struct Animation
        {
                std::string name;
                std::vector<AnimationSampler> samplers;
                std::vector<AnimationChannel> channels;
                float start = std::numeric_limits<float>::max();
                float end   = std::numeric_limits<float>::min();
        };

        enum FileLoadingFlags
        {
                None                    = 0x00000000,
                PreTransformVertices    = 0x00000001,
                PreMultiplyVertexColors = 0x00000002,
                FlipY                   = 0x00000004,
                DontLoadImages          = 0x00000008
        };

        enum RenderFlags
        {
                BindImages              = 0x00000001,
                RenderOpaqueNodes       = 0x00000002,
                RenderAlphaMaskedNodes  = 0x00000004,
                RenderAlphaBlendedNodes = 0x00000008
        };

        enum DescriptorBindingFlags
        {
                ImageBaseColor = 0x00000001,
                ImageNormalMap = 0x00000002
        };

        /** @brief Before understandiing the Model that loads the buffer we have to understand gltf as a whole. Since
         * for now this is just a gltf loader and that is the standard loader and any other loader would later be
         * implemented in the API using some kind of overlaoding function. So gltf contains buffers. Buffers are just
         * binary data. The binary data is just an array of bytes. The bytes can be accessed through bufferviews. Which
         * just contains the position of each kind of data. Vertices indices etc. Its offset to find that data. Now
         * bufferview itself has no meaning accept for addressing. TO actually able to understand that data we have
         * accessors. These reference the index of an bufferview. Define it type to access it its size and count for the
         * gltf to be understaod. The gltf library is based on that and is basically just parsing the json object and
         * converting the binary data to readable data and accessing it through the binary. Now its our turn to load it
         * into the Scene graph. */

        extern VkDescriptorSetLayout descriptorSetLayoutImage;
        extern VkDescriptorSetLayout descriptorSetLayoutUbo;
        extern VkMemoryPropertyFlags memoryPropertyFlags;
        extern uint32_t descriptorBindingFlags;

        struct Model
        {

                ic::vkdevice* device;
                VkDescriptorPool descriptorPool;  // TODO desciptors need to be setup in descriptors section not in
                                                  // model
                // but at some point I am going to make my own model loader so this is just for practice.
                // Honestly speaking this part of the engine is for planning out the engine. I am going to test out
                // different features and going to plan them out in the engine as I move forward.
                // My main goal is to be able to render cool graphics. Once that is done in vulkan I am gonna do the
                // same for OpenGL. Once that is done I am going to plan out my engine and write the overall classes and
                // loading for the engine with UI inbuilt.

                /** @brief A vertex is just location of vertex for connections in 3D representation that computer (we in
                 * the vulkan) converts it into the screen coordinates Ussually a position and RGBA values suffices. But
                 * for materials and textures we have normals and uvs. Joints and weights are for animation and physics.
                 */
                struct alignas(16) Vertex
                {
                        glm::vec3 pos;
                        glm::vec3 normal;
                        glm::vec2 uv0;
                        glm::vec2 uv1;
                        glm::uvec4 joint0;
                        glm::vec4 weight0;
                        glm::vec4 color;
                        glm::vec4 tangent;
                };

                struct Vertices
                {
                        VkBuffer buffer = VK_NULL_HANDLE;
                        VkDeviceMemory memory;
                } vertices;

                struct Indices
                {
                        VkBuffer buffer = VK_NULL_HANDLE;
                        VkDeviceMemory memory;
                } indices;

                glm::mat4 aabb;

                std::vector<Node*> nodes;
                std::vector<Node*> linearNodes;

                std::vector<Skin*> skins;

                std::vector<Texture> textures;
                std::vector<TextureSampler> textureSamplers;
                std::vector<Material> materials;
                std::vector<Animation> animations;
                std::vector<std::string> extensions;

                struct Dimensions
                {
                        glm::vec3 min = glm::vec3(FLT_MAX);
                        glm::vec3 max = glm::vec3(-FLT_MAX);
                } dimensions;

                struct LoaderInfo
                {
                        uint32_t* indexBuffer;
                        Vertex* vertexBuffer;
                        size_t indexPos  = 0;
                        size_t vertexPos = 0;
                };

                std::string filePath;

                void destroy(VkDevice device);

                // TODO use fastGLTF instead
                void loadNode(vkLoad::Node* parent,
                              const tinygltf::Node& node,
                              uint32_t nodeIndex,
                              const tinygltf::Model& model,
                              LoaderInfo& loaderInfo,
                              float globalscale);
                void getNodeProps(const tinygltf::Node& node,
                                  const tinygltf::Model& model,
                                  size_t& vertexCount,
                                  size_t& indexCount);
                void loadImages(tinygltf::Model& gltfModel, ic::vkdevice* device, VkQueue transferQueue);
                void loadSkins(tinygltf::Model& gltfModel);
                void loadTextures(tinygltf::Model& gltfModel, ic::vkdevice* device, VkQueue transferQueue);
                VkSamplerAddressMode getVkWrapMode(int32_t wrapMode);
                VkFilter getVkFilterMode(int32_t filterMode);
                void loadTextureSamplers(tinygltf::Model& gltfModel);
                void loadMaterials(tinygltf::Model& gltfModel);
                void loadAnimations(tinygltf::Model& gltfModel);
                void loadFromFile(std::string filename,
                                  ic::vkdevice* device,
                                  VkQueue transferQueue,
                                  uint32_t fileLoadingFlags,
                                  float scale = 1.0f);
                void drawNode(Node* node, VkCommandBuffer commandBuffer);
                void draw(VkCommandBuffer commandBuffer);
                void calculateBoundingBox(Node* node, Node* parent);
                void getSceneDimensions();
                void updateAnimation(uint32_t index, float time);
                Node* findNode(Node* parent, uint32_t index);
                Node* nodeFromIndex(uint32_t index);
                void prepareNodeDescriptor(Node* node, VkDescriptorSetLayout descriptorSetLayout);
        };

}  // namespace vkLoad
