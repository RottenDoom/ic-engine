#include "framebuffer.h"

namespace ic
{
        framebuffer::~framebuffer() {}

        framebuffer::framebuffer(framebuffer&& other) noexcept
        {
                moveFrom(std::move(other));
        }

        framebuffer& framebuffer::operator=(framebuffer&& other) noexcept
        {
                if (this != &other)
                {
                        // TODO fix this memory leak
                        reset();
                        moveFrom(std::move(other));
                }
                return *this;
        }

        bool framebuffer::create(const VkDevice& device, const VkRenderPass& renderpass,
                                 const VkExtent2D& swapChainExtent, const uint32_t& attachmentCount,
                                 const VkImageView* pAttachments, VkAllocationCallbacks* callbacks) noexcept
        {
                VkFramebufferCreateInfo CI{};
                CI.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                CI.renderPass      = renderpass;
                CI.attachmentCount = attachmentCount;
                CI.pAttachments    = pAttachments;
                CI.width           = swapChainExtent.width;
                CI.height          = swapChainExtent.height;
                CI.layers          = 1;

                // TODO: allocator callbacks and maybe each index info
                if (vkCreateFramebuffer(device, &CI, callbacks, &m_framebuffer) != VK_SUCCESS)
                {
                        IC_CORE_ERROR("Failed to create Framebuffer");
                        return false;
                }

                return true;
        }

        void framebuffer::destroy(const VkDevice& device)
        {
                if (m_framebuffer != VK_NULL_HANDLE)
                {
                        vkDestroyFramebuffer(device, m_framebuffer, nullptr); // TODO: allocator
                }
                reset();
        }

        void framebuffer::moveFrom(framebuffer&& other)
        {
                m_framebuffer = other.m_framebuffer;
        }

        void framebuffer::reset()
        {
                m_framebuffer = VK_NULL_HANDLE;
        }

} // namespace ic
