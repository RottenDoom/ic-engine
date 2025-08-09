#include "queue_manager.h"

namespace ic
{
    queue_manager::queue_manager(queue_manager&& other) noexcept {
        m_device         = other.m_device;
        m_indices        = other.m_indices;
        m_graphicsQueue  = other.m_graphicsQueue;
        m_presentQueue   = other.m_presentQueue;
        m_computeQueue   = other.m_computeQueue;
        m_transferQueue  = other.m_transferQueue;
    
        // Reset other's state
        other.m_device = VK_NULL_HANDLE;
        other.m_graphicsQueue = VK_NULL_HANDLE;
        other.m_presentQueue = VK_NULL_HANDLE;
        other.m_computeQueue = VK_NULL_HANDLE;
        other.m_transferQueue = VK_NULL_HANDLE;
    }
    
    queue_manager& queue_manager::operator=(queue_manager&& other) noexcept {
        if (this != &other) {
            m_device         = other.m_device;
            m_indices        = other.m_indices;
            m_graphicsQueue  = other.m_graphicsQueue;
            m_presentQueue   = other.m_presentQueue;
            m_computeQueue   = other.m_computeQueue;
            m_transferQueue  = other.m_transferQueue;
    
            // Reset other's state
            other.m_device = VK_NULL_HANDLE;
            other.m_graphicsQueue = VK_NULL_HANDLE;
            other.m_presentQueue = VK_NULL_HANDLE;
            other.m_computeQueue = VK_NULL_HANDLE;
            other.m_transferQueue = VK_NULL_HANDLE;
        }
        return *this;
    }

    bool queue_manager::initialize(VkDevice device, const queue_family_indices& indices) {
        if (device == VK_NULL_HANDLE || !indices.isComplete()) {
            return false;
        }

        m_device  = device;
        m_indices = indices;

        vkGetDeviceQueue(m_device, m_indices.graphicsFamily.value(), 0, &m_graphicsQueue);
        vkGetDeviceQueue(m_device, m_indices.presentFamily.value(), 0, &m_presentQueue);

        if (m_indices.computeFamily.has_value()) {
            vkGetDeviceQueue(m_device, m_indices.computeFamily.value(), 0, &m_computeQueue);
        }
        if (m_indices.transferFamily.has_value()) {
            vkGetDeviceQueue(m_device, m_indices.transferFamily.value(), 0, &m_transferQueue);
        }

        return true;
    }

    void queue_manager::waitIdle() const {
        if (m_device != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(m_device);
        }
    }

    void queue_manager::waitGraphicsQueue() const {
        if (m_graphicsQueue != VK_NULL_HANDLE) {
            vkQueueWaitIdle(m_graphicsQueue);
        }
    }

    void queue_manager::waitPresentQueue() const {
        if (m_presentQueue != VK_NULL_HANDLE) {
            vkQueueWaitIdle(m_presentQueue);
        }
    }

    queue_family_indices queue_manager::findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
        queue_family_indices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            // graphics
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }
            // compute
            if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
                indices.computeFamily = i;
            }
            // transfer (dedicated if possible)
            if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT &&
                !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                indices.transferFamily = i;
            }
            // present support
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport) {
                indices.presentFamily = i;
            }

            if (indices.isComplete()) break;
            i++;
        }

        // if no dedicated transfer found, fall back to graphics
        if (!indices.transferFamily.has_value()) {
            indices.transferFamily = indices.graphicsFamily;
        }

        // if no compute found, fall back to graphics
        if (!indices.computeFamily.has_value()) {
            indices.computeFamily = indices.graphicsFamily;
        }

        return indices;
    }
} // namespace ic
