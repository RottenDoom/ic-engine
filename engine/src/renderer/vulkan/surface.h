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

        vulkan_surface(const vulkan_surface&) = delete;
        vulkan_surface& operator=(const vulkan_surface&) = delete;
        vulkan_surface(vulkan_surface&& other) noexcept;
        vulkan_surface& operator=(vulkan_surface&& other) noexcept;

        bool create(VkInstance* instance,
                    GLFWwindow* window); // VkResult result = glfwCreateWindowSurface(*m_vk_instance, window, nullptr,
                                         // m_surface->get());
        void destroy(VkInstance* instance);

        VkSurfaceKHR get() const
        {
            return m_surface;
        }
        // operator VkSurfaceKHR() const { return m_surface; } // TODO: how does this work

        bool isValid() const
        {
            return m_surface != VK_NULL_HANDLE;
        }
    };

} // namespace ic