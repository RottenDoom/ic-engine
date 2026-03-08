#pragma once
#include "core/window.h"
#include "device.h"
#include "renderpass.h"
#include "surface.h"
#include "physical_device.h"
#include "swapchain.h"

namespace ic
{
        class vulkan_context
        {
        public:
                VkPhysicalDeviceFeatures features{
                    .samplerAnisotropy = VK_TRUE,
                };

                std::vector<const char*> extensions = {
                    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                };

        private:
                bool m_initialized = false;

                std::unique_ptr<VkInstance> m_vk_instance;
                std::unique_ptr<vulkan_surface> m_surface;
                std::unique_ptr<vkdevice> m_device;
                Window& m_window;

                VkDebugUtilsMessengerEXT m_debugMessenger   = VK_NULL_HANDLE;

                std::vector<const char*> m_validationLayers = {"VK_LAYER_KHRONOS_validation"};

        public:
                struct context_settings
                {
                        bool validation = true;
                        bool fullscreen = false;
                        bool vsync      = false;
                        bool overlay    = false;

                } settings;

                vulkan_context(Window& w);

                // Move semantics
                vulkan_context(const vulkan_context&)            = delete;
                vulkan_context& operator=(const vulkan_context&) = delete;
                vulkan_context(vulkan_context&& other) noexcept;
                vulkan_context& operator=(vulkan_context&& other) noexcept;

                virtual ~vulkan_context();

                bool initialize(bool enableValidation = true);

                void cleanUp();

                vulkan_surface getSurface() const { return *m_surface; }
                vkdevice& getVulkanDevice() const { return *m_device; }
                VkDevice getDevice() const { return m_device->logicalDevice; }
                VkPhysicalDevice getPhysicalDevice() const { return m_device->physicalDevice; }

                VkInstance* getInstance() const { return m_vk_instance.get(); }
                VkExtent2D getWindowExtent() const
                {
                        VkExtent2D extentOut;
                        extentOut.width  = m_window.getWidth();
                        extentOut.height = m_window.getHeight();
                        return extentOut;
                }

                static vulkan_context* s_context;
                static vulkan_context* get() { return s_context; }  // getter for context;

        private:
                bool createInstance();
                bool createSurface(GLFWwindow* window);
                bool createDevice();
        };
}  // namespace ic
