#pragma once
#include "defines.h"
#include "context.h"
#include "sync_objects.h"
#include "renderer/camera.h"
#include "buffer.h"
#include "pipeline_config.h"
#include "framebuffer.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace ic
{
        class vulkan_renderer
        {

        public:
                /** @brief The basic uniform buffer structs and uniform buffer */
                struct Vertex
                {
                        glm::vec3 pos;
                        glm::vec3 color;
                        // glm::vec3 normal;
                        // glm::vec2 uv;
                        // glm::vec4 tangent;

                        static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
                        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
                };

                const std::vector<Vertex> vertices = {
                    // Face 1: Front (Z = -1.0f) - Color: Red (1, 0, 0)
                    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},  // 0: Bottom-left
                    {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},   // 1: Bottom-right
                    {{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},    // 2: Top-right
                    {{-0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},   // 3: Top-left

                    // Face 2: Back (Z = +0.5f) - Color: Green (0, 1, 0)
                    {{-0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},  // 4: Bottom-left
                    {{0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},   // 5: Bottom-right
                    {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},    // 6: Top-right
                    {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},   // 7: Top-left

                    // Face 3: Right (X = +0.5f) - Color: Blue (0, 0, 1)
                    {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},  // 8: Bottom-back (duplicate of 1)
                    {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},   // 9: Bottom-front (duplicate of 5)
                    {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},    // 10: Top-front (duplicate of 6)
                    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},   // 11: Top-back (duplicate of 2)

                    // Face 4: Left (X = -0.5f) - Color: Yellow (1, 1, 0)
                    {{-0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 0.0f}},   // 12: Bottom-front (duplicate of 4)
                    {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}},  // 13: Bottom-back (duplicate of 0)
                    {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}},   // 14: Top-back (duplicate of 3)
                    {{-0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 0.0f}},    // 15: Top-front (duplicate of 7)

                    // Face 5: Top (Y = +0.5f) - Color: Cyan (0, 1, 1)
                    {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},  // 16: Front-left (duplicate of 3)
                    {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},   // 17: Front-right (duplicate of 2)
                    {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 1.0f}},    // 18: Back-right (duplicate of 6)
                    {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 1.0f}},   // 19: Back-left (duplicate of 7)

                    // Face 6: Bottom (Y = -0.5f) - Color: Magenta (1, 0, 1)
                    {{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 1.0f}},  // 20: Back-left (duplicate of 4)
                    {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 1.0f}},   // 21: Back-right (duplicate of 5)
                    {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}},  // 22: Front-right (duplicate of 1)
                    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}}  // 23: Front-left (duplicate of 0)
                };

                const std::vector<uint16_t> indices = {
                    // Face 1: Front (Vertices 0-3)
                    0,
                    1,
                    2,
                    2,
                    3,
                    0,
                    // Face 2: Back (Vertices 4-7)
                    4,
                    5,
                    6,
                    6,
                    7,
                    4,
                    // Face 3: Right (Vertices 8-11)
                    8,
                    9,
                    10,
                    10,
                    11,
                    8,
                    // Face 4: Left (Vertices 12-15)
                    12,
                    13,
                    14,
                    14,
                    15,
                    12,
                    // Face 5: Top (Vertices 16-19)
                    16,
                    17,
                    18,
                    18,
                    19,
                    16,
                    // Face 6: Bottom (Vertices 20-23)
                    20,
                    21,
                    22,
                    22,
                    23,
                    20};

                const uint32_t indexCount = indices.size();

                struct UniformData
                {
                        glm::mat4 projection;
                        glm::mat4 modelMatrix;
                        glm::mat4 viewMatrix;
                        // glm::vec4 lightPos{0.0f, 2.0f, 1.0f, 0.0f};
                } uniformData;
                std::vector<buffer> uniformBuffers;
                buffer vertexBuffer;
                buffer indexBuffer;

                /** @brief For including device and other context related stuff. */
                vulkan_context* context   = nullptr;
                uint32_t maxFrameInFlight = 3;  // fix these in some constant file
                VkPhysicalDeviceFeatures deviceFeatures{};

                /** @brief pipeline */
                pipeline_config config;
                VkPipelineCache pipelineCache;
                VkPhysicalDeviceFeatures enabledFeatures{};
                struct
                {
                        VkPipeline phong{VK_NULL_HANDLE};
                        VkPipeline wireFrame{VK_NULL_HANDLE};
                        VkPipeline toon{VK_NULL_HANDLE};
                } pipelines;
                std::vector<framebuffer> swapchainFramebuffers;

                /** @brief command pool and command buffers */
                VkCommandPool cmdPool{};
                std::vector<VkCommandBuffer> commandBuffers;

                /** @brief Descriptors for the renderer */
                std::vector<VkDescriptorSet> descriptorSets;
                VkDescriptorPool descriptorPool;
                VkPipelineLayout pipelineLayout{VK_NULL_HANDLE};
                VkDescriptorSetLayout descriptorSetLayout{VK_NULL_HANDLE};
                std::vector<VkCommandBuffer> drawCmdBuffers;

                /** @brief Stencil Properties */
                VkQueue queue;
                struct
                {
                        VkImage image;
                        VkDeviceMemory memory;
                        VkImageView view;
                } depthStencil;
                VkExtent2D extent2d;
                VkFormat depthFormat;
                uint32_t currentBuffer{0};
                uint32_t currentImageIndex{0};
                VkClearColorValue defaultClearColor = {{0.025f, 0.025f, 0.025f, 1.0f}};

                std::vector<VkSemaphore> presentSemaphores{};  // waits for completion of presenting
                std::vector<VkSemaphore> renderSemaphore{};    // waits for completion of rendering
                std::vector<VkFence> waitFences{};

        private:
                bool m_prepared = false;

                /** @brief Camera for now is just a basic implementation with events later multiple camera types will be
                 * available that I might use with this engine outside just the renderer but with scenes */
                Camera camera;

        public:
                explicit vulkan_renderer(vulkan_context* pContext) noexcept;

                bool init();
                void onEvent(event& e);
                void onUpdate(float deltaTime);  // put this function somewhere else
                void render(float deltaTime);
                void destroy();

                void windowResize();
                void getEnabledFeatures();

        private:
                void loadAssets();  // this should be an api for users to use (somehow)
                void setupDescriptors(VkDevice& device);
                void createCommandPool();
                void createCommandBuffers(VkDevice& device);

                void createVertexBuffer();
                void createIndexedBuffer();

                void createGraphicsPipeline(VkDevice& device);
                void setupDepthStencil(VkDevice& device);
                void createFramebuffers(VkDevice& device);
                void createSyncObjects(VkDevice& device);

                void prepareFrame(VkDevice& device, bool waitForFence = true);
                void submitFrame(VkDevice& device, bool skipQueueSubmit = false);
                void buildCommandBuffers();
                void prepareUniformBuffers();
                void updateUniformBuffers();
        };
}  // namespace ic
