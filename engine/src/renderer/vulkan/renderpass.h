#pragma once
#include "defines.h"

namespace ic
{
        class render_pass
        {
        private:
                VkRenderPass m_renderPass; // maybe make this a pointer
                VkFormat m_swapchainImageFormat;

        public:
                render_pass() = default;
                virtual ~render_pass();

                render_pass(const render_pass&)            = delete;
                render_pass& operator=(const render_pass&) = delete;
                render_pass(const render_pass&& other) noexcept;
                render_pass& operator=(const render_pass&& other) noexcept;

                VkRenderPass get() const { return m_renderPass; }

                // TODO: fix these functions with a allocator
                bool create(VkDevice device, const VkFormat& swapchainImageForamt, const VkFormat& depthFormat,
                            const VkAllocationCallbacks* callbacks = nullptr);
                void destroy(VkDevice device, const VkAllocationCallbacks* callbacks = nullptr);

        private:
                void moveFrom(const render_pass&& other) noexcept;
                void reset() noexcept;
        };
} // namespace ic
