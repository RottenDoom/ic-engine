#include "surface.h"

namespace ic
{
        vulkan_surface::~vulkan_surface()
        {
                // check for memory leaks
        }

        bool vulkan_surface::create(VkInstance* instance, GLFWwindow* window)
        {
                if (m_surface != VK_NULL_HANDLE)
                {
                        IC_CORE_WARN("Surface already created");
                        return true;
                }

                if (glfwCreateWindowSurface(*instance, window, nullptr, &m_surface) != VK_SUCCESS)
                {
                        IC_CORE_ERROR("GLFW Surface Creation Failed!");
                        return false;
                }
                return true;
        }

        void vulkan_surface::destroy(VkInstance* instance)
        {
                if (m_surface != VK_NULL_HANDLE)
                {
                        vkDestroySurfaceKHR(*instance, m_surface, nullptr);
                        m_surface = VK_NULL_HANDLE;
                }
        }
}  // namespace ic
