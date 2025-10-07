#include "vkcmd.h"

namespace ic
{
        void command_buffers::moveFrom(command_buffers&& other)
        {
                m_commandBuffers = other.m_commandBuffers;
                m_commandBuffers.clear();
        }

        command_buffers::command_buffers(command_buffers&& other)
        {
                moveFrom(std::move(other));
        }

        command_buffers& command_buffers::operator=(command_buffers&& other)
        {
                if (this != &other)
                        moveFrom(std::move(other));
                return *this;
        }

        bool command_buffers::allocate(const VkDevice& device, const VkCommandPool& cmdPool,
                                       const uint32_t& maxFramesInFlight, VkCommandBufferLevel level)
        {
                m_commandBuffers.resize(maxFramesInFlight);
                VkCommandBufferAllocateInfo allocInfo{};
                allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                allocInfo.commandPool        = cmdPool;
                allocInfo.level              = level;
                allocInfo.commandBufferCount = m_commandBuffers.size();

                return vkAllocateCommandBuffers(device, &allocInfo, m_commandBuffers.data()) == VK_SUCCESS;
        }

        void command_buffers::begin(VkCommandBuffer& cmdBuffer, VkCommandBufferUsageFlags flags) const
        {
                VkCommandBufferBeginInfo beginInfo{};
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                beginInfo.flags = flags;
                vkBeginCommandBuffer(cmdBuffer, &beginInfo);
        }

        void command_buffers::end(VkCommandBuffer& cmdBuffer) const
        {
                vkEndCommandBuffer(cmdBuffer);
        }

        void command_buffers::free(const VkDevice& device, const VkCommandPool& cmdPool,
                                   VkCommandBuffer& cmdBuffer) noexcept
        {
                if (cmdBuffer != VK_NULL_HANDLE)
                {
                        vkFreeCommandBuffers(device, cmdPool, 1, &cmdBuffer);
                }
        }

} // namespace ic
