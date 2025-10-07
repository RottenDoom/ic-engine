#pragma once
#include "defines.h"
#include "queue_manager.h"

namespace ic
{
        struct command_pool
        {
                VkCommandPool commandPool;
                queue_family_indices queueFamilyIndices;

                void create(const VkDevice& device, const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface)
                {
                        queueFamilyIndices = queue_manager::findQueueFamilies(physicalDevice, surface);
                        VkCommandPoolCreateInfo CI{};
                        CI.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
                        CI.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // transient or reset
                        CI.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

                        // TODO: Allocator callbacks
                        if (vkCreateCommandPool(device, &CI, nullptr, &commandPool) != VK_SUCCESS)
                        {
                                IC_CORE_ERROR("Failed to create command pool!");
                        };
                }

                void destroy(const VkDevice& device)
                {
                        vkDestroyCommandPool(device, commandPool, nullptr);
                        commandPool = VK_NULL_HANDLE;
                }
        };

        // TODO: make a better wrapper
        class command_buffers
        {
        private:
                std::vector<VkCommandBuffer> m_commandBuffers;

        public:
                command_buffers()                                  = default;
                virtual ~command_buffers()                         = default;

                command_buffers(const command_buffers&)            = delete;
                command_buffers& operator=(const command_buffers&) = delete;
                command_buffers(command_buffers&& other);
                command_buffers& operator=(command_buffers&& other);

                bool allocate(const VkDevice& device, const VkCommandPool& cmdPool, const uint32_t& maxFramesInFlight,
                              VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

                std::vector<VkCommandBuffer> get() const { return m_commandBuffers; }

                void begin(VkCommandBuffer& cmdBuffer, VkCommandBufferUsageFlags flags = 0) const;
                void end(VkCommandBuffer& cmdBuffer) const;

                void free(const VkDevice& device, const VkCommandPool& cmdPool, VkCommandBuffer& cmdBuffer) noexcept;

        private:
                void moveFrom(command_buffers&& other);
        };
} // namespace ic
