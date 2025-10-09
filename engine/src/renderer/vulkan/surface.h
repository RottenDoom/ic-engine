#pragma once
#include "defines.h"

struct GLFWwindow;

namespace ic
{

        class vulkan_surface
        {
        private:
                VkSurfaceKHR m_surface;

        public:
                vulkan_surface() = default;
                ~vulkan_surface();

                bool create(VkInstance* instance,
                            GLFWwindow* window);  // VkResult result = glfwCreateWindowSurface(*m_vk_instance, window,
                                                  // nullptr, m_surface->get());
                void destroy(VkInstance* instance);

                operator VkSurfaceKHR() const { return m_surface; }  // TODO: how does this work
                VkSurfaceKHR& get() { return m_surface; }

                bool isValid() const { return m_surface != VK_NULL_HANDLE; }
        };

}  // namespace ic