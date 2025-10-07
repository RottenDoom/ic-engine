#pragma once
#include "context.h"
#include "defines.h"
#include "framebuffer.h"
#include "ibuffer.h"
#include "pipeline.h"
#include "pipeline_layout.h"
#include "sync_objects.h"
#include "vkcmd.h"

namespace ic
{
        const std::vector<vertex> vertices = {{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                                              {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
                                              {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
                                              {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}};

        void createVertexBuffers(logical_device& device, VkCommandPool& cmdPool, VkQueue& queue,
                                 const std::vector<vertex>& vertices, VkBuffer& buffer, VkDeviceMemory& memory);

        // class vertex_buffer : public ibuffer
        // {
        //         vertex_buffer(VkDeviceSize& size, VkBuffer& buffer, VkDeviceMemory& memory);

        // private:
        //         VkDeviceSize m_bufferSize;
        //         VkBuffer m_stagingBuffer;
        //         VkDeviceMemory m_staginBufferMemory;
        // };

        class vulkan_renderer
        {
        private:
                void moveFrom(vulkan_renderer&& other) noexcept;
                void reset() noexcept;

        public:
                vulkan_renderer() = default;
                virtual ~vulkan_renderer();

                vulkan_renderer(const vulkan_renderer&)            = delete;
                vulkan_renderer& operator=(const vulkan_renderer&) = delete;
                vulkan_renderer(vulkan_renderer&& other) noexcept;
                vulkan_renderer& operator=(vulkan_renderer&& other) noexcept;

                bool create() noexcept;
                void drawFrame() noexcept;
                void destroy(const VkDevice& device);

        private:
                pipeline_config m_config;
                pipeline_layout m_layout;
                std::unique_ptr<pipeline> m_pipeline;
                std::vector<framebuffer*> m_swapchainFramebuffers;
                std::vector<VkDescriptorSet> m_descriptorSets;
                VkBuffer vertexBuffer;
                VkDeviceMemory vertexBufferMemory;
                command_pool m_cmdPool{};
                command_buffers m_cmdBuffers;
                descriptor_pool m_descriptorPool;
                fences m_fences{};
                semaphores m_semaphores{};

                uint32_t maxFrameInFlight = 3; // fix these in some constant file

        private:
                void createFramebuffers(const vulkan_context* context,
                                        const std::vector<VkImageView>& swapChainImageViews);
        };
} // namespace ic
