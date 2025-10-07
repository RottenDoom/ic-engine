#pragma once
#include "defines.h"

namespace ic
{
        class framebuffer
        {
        private:
                VkFramebuffer m_framebuffer;

        public:
                framebuffer() = default;
                virtual ~framebuffer();

                framebuffer(const framebuffer&)            = delete;
                framebuffer& operator=(const framebuffer&) = delete;
                framebuffer(framebuffer&& other) noexcept;
                framebuffer& operator=(framebuffer&& other) noexcept;

                bool create(const VkDevice& device, const VkRenderPass& renderpass, const VkExtent2D& swapChainExtent,
                            const uint32_t& attachmentCount, const VkImageView* pAttachments,
                            VkAllocationCallbacks* callbacks = nullptr) noexcept;

                void destroy(const VkDevice& device);

        private:
                void moveFrom(framebuffer&& other);
                void reset();
        };
} // namespace ic
