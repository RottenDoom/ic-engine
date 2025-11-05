#pragma once
#include "defines.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

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

                bool create(VkInstance* instance, GLFWwindow* window);
                void destroy(VkInstance* instance);

                operator VkSurfaceKHR() const { return m_surface; }
                VkSurfaceKHR& get() { return m_surface; }

                bool isValid() const { return m_surface != VK_NULL_HANDLE; }
        };

}  // namespace ic