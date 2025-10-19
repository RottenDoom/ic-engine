#pragma once
#include "defines.h"

namespace ic
{
        class RenderPass
        {
        private:
                VkRenderPass m_renderPass;  // maybe make this a pointer
                VkFormat m_swapchainImageFormat;

        public:
                RenderPass() = default;
                virtual ~RenderPass();

                RenderPass(const RenderPass&)            = delete;
                RenderPass& operator=(const RenderPass&) = delete;
                RenderPass(const RenderPass&& other) noexcept;
                RenderPass& operator=(const RenderPass&& other) noexcept;

                VkRenderPass get() const { return m_renderPass; }

                // TODO: fix these functions with a allocator
                bool create(VkDevice device,
                            const VkFormat& swapchainImageForamt,
                            const VkFormat& depthFormat,
                            const VkAllocationCallbacks* callbacks = nullptr);
                void destroy(VkDevice device, const VkAllocationCallbacks* callbacks = nullptr);

        private:
                void moveFrom(const RenderPass&& other) noexcept;
                void reset() noexcept;
        };
}  // namespace ic
