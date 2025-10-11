#pragma once
#include "defines.h"
#include "context.h"
#include "sync_objects.h"
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
                    {{0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},  // Bottom-left (Red)
                    {{0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},   // Bottom-right (Green)
                    {{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}   // Top-center (Blue)
                };

                struct UniformData
                {
                        glm::mat4 projection;
                        glm::mat4 modelView;
                        glm::vec4 lightPos{0.0f, 2.0f, 1.0f, 0.0f};
                        // glm::mat4 viewMatrix;
                } uniformData;
                std::vector<buffer> uniformBuffers;
                buffer vertexBuffer;

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

        public:
                explicit vulkan_renderer(vulkan_context* pContext) noexcept;

                bool init();
                void render();
                void destroy();

                void windowResize();
                void getEnabledFeatures();

        private:
                void loadAssets();  // this should be an api for users to use (somehow)
                void setupDescriptors(VkDevice& device);
                void createCommandPool();
                void createCommandBuffers(VkDevice& device);
                void createVertexBuffer();
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
