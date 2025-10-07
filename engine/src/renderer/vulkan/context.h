#pragma once
#include "core/window.h"
#include "logical_device.h"
#include "pipeline.h"
#include "renderpass.h"
#include "surface.h"
#include "swapchain.h"

// TODO: fix const references

namespace ic
{
        class vulkan_context
        {
        private:
                bool m_initialized = false;

                std::unique_ptr<VkInstance> m_vk_instance;
                std::unique_ptr<vulkan_surface> m_surface;
                std::unique_ptr<logical_device> m_device;
                std::unique_ptr<swapchain> m_swapchain;
                std::unique_ptr<render_pass> m_renderpass;

                VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

// debug utils (TODO: make this global setting)
#ifdef _DEBUG
                bool m_enableValidation = true;
#else
                bool m_enableValidation = false;
#endif
                const std::vector<const char*> m_validationLayers = {"VK_LAYER_KHRONOS_validation"};

        public:
                vulkan_context() = default;

                // Move semantics
                vulkan_context(const vulkan_context&)            = delete;
                vulkan_context& operator=(const vulkan_context&) = delete;
                vulkan_context(vulkan_context&& other) noexcept;
                vulkan_context& operator=(vulkan_context&& other) noexcept;

                virtual ~vulkan_context();

                bool initialize(GLFWwindow* window, bool enableValidation = true);

                void cleanUp();
                void recreateSwapChain();

                vulkan_surface* getSurface() const { return m_surface.get(); }
                logical_device* getDevice() const { return m_device.get(); }
                swapchain* getSwapChain() const { return m_swapchain.get(); }
                render_pass* getRenderpass() const { return m_renderpass.get(); }

                // void waitIdle() { if (m_device) m_device->waitIdle(); } // TODO get this somewhere else.

                VkInstance* getInstance() const { return m_vk_instance.get(); }

                static vulkan_context* s_context;
                static vulkan_context* get() { return s_context; } // getter for context;

        private:
                bool createInstance();
                bool createSurface(GLFWwindow* window);
                bool createDevice();
                bool createSwapChain();
                bool createRenderPass();
        };
} // namespace ic
