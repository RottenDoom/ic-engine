#include "queue_manager.h"

namespace ic
{
    queue_manager::queue_manager(queue_manager&& other) noexcept
        : m_device(std::exchange(other.m_device, VK_NULL_HANDLE)), m_indices(std::move(other.m_indices)),
          m_graphicsQueue(std::exchange(other.m_graphicsQueue, VK_NULL_HANDLE)),
          m_presentQueue(std::exchange(other.m_presentQueue, VK_NULL_HANDLE)),
          m_computeQueue(std::exchange(other.m_computeQueue, VK_NULL_HANDLE)),
          m_transferQueue(std::exchange(other.m_transferQueue, VK_NULL_HANDLE))
    {
    }

    queue_manager& queue_manager::operator=(queue_manager&& other) noexcept
    {
        if (this != &other)
        {
            cleanup();

            m_device = std::exchange(other.m_device, VK_NULL_HANDLE);
            m_indices = std::move(other.m_indices);
            m_graphicsQueue = std::exchange(other.m_graphicsQueue, VK_NULL_HANDLE);
            m_presentQueue = std::exchange(other.m_presentQueue, VK_NULL_HANDLE);
            m_computeQueue = std::exchange(other.m_computeQueue, VK_NULL_HANDLE);
            m_transferQueue = std::exchange(other.m_transferQueue, VK_NULL_HANDLE);
        }
        return *this;
    }

    queue_manager::~queue_manager()
    {
        cleanup();
    }

    void queue_manager::cleanup()
    {
        if (m_device != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(m_device);
        }

        m_device = VK_NULL_HANDLE;
        m_graphicsQueue = VK_NULL_HANDLE;
        m_presentQueue = VK_NULL_HANDLE;
        m_computeQueue = VK_NULL_HANDLE;
        m_transferQueue = VK_NULL_HANDLE;

        m_indices = queue_family_indices{};
    }

    bool queue_manager::initialize(VkDevice device, const queue_family_indices& indices)
    {
        if (device == VK_NULL_HANDLE)
        {
            IC_CORE_ERROR("Invalid device handle");
            return false;
        }

        if (!indices.isComplete())
        {
            IC_CORE_ERROR("Incomplete queue family indices");
            return false;
        }

        cleanup();

        m_device = device;
        m_indices = indices;

        vkGetDeviceQueue(m_device, m_indices.graphicsFamily.value(), 0, &m_graphicsQueue);
        vkGetDeviceQueue(m_device, m_indices.presentFamily.value(), 0, &m_presentQueue);

        if (m_graphicsQueue == VK_NULL_HANDLE || m_presentQueue == VK_NULL_HANDLE)
        {
            IC_CORE_ERROR("Failed to get required queues");
            cleanup();
            return false;
        }

        // Get optional queues
        if (m_indices.computeFamily.has_value())
        {
            vkGetDeviceQueue(m_device, m_indices.computeFamily.value(), 0, &m_computeQueue);
            IC_CORE_TRACE("Compute queue initialized: family {}", m_indices.computeFamily.value());
        }

        if (m_indices.transferFamily.has_value())
        {
            vkGetDeviceQueue(m_device, m_indices.transferFamily.value(), 0, &m_transferQueue);
            IC_CORE_TRACE("Transfer queue initialized: family {}", m_indices.transferFamily.value());
        }

        IC_CORE_INFO("Queue Manager initialized successfully");
        return true;
    }

    bool queue_manager::isInitialized() const
    {
        return m_device != VK_NULL_HANDLE && m_graphicsQueue != VK_NULL_HANDLE && m_presentQueue != VK_NULL_HANDLE;
    }

    void queue_manager::waitIdle() const
    {
        if (m_device != VK_NULL_HANDLE)
        {
            VkResult result = vkDeviceWaitIdle(m_device);
            if (result != VK_SUCCESS)
            {
                IC_CORE_WARN("vkDeviceWaitIdle failed with result {}", static_cast<int>(result));
            }
        }
    }

    void queue_manager::waitGraphicsQueue() const
    {
        if (m_graphicsQueue != VK_NULL_HANDLE)
        {
            VkResult result = vkQueueWaitIdle(m_graphicsQueue);
            if (result != VK_SUCCESS)
            {
                IC_CORE_WARN("vkQueueWaitIdle failed with result {}", static_cast<int>(result));
            }
        }
    }

    void queue_manager::waitPresentQueue() const
    {
        if (m_presentQueue != VK_NULL_HANDLE)
        {
            VkResult result = vkQueueWaitIdle(m_presentQueue);
            if (result != VK_SUCCESS)
            {
                IC_CORE_WARN("vkQueueWaitIdle failed with result {}", static_cast<int>(result));
            }
        }
    }

    void queue_manager::waitComputeQueue() const
    {
        if (m_computeQueue != VK_NULL_HANDLE)
        {
            VkResult result = vkQueueWaitIdle(m_computeQueue);
            if (result != VK_SUCCESS)
            {
                IC_CORE_WARN("vkQueueWaitIdle failed with result {}", static_cast<int>(result));
            }
        }
    }

    void queue_manager::waitTransferQueue() const
    {
        if (m_transferQueue != VK_NULL_HANDLE)
        {
            VkResult result = vkQueueWaitIdle(m_transferQueue);
            if (result != VK_SUCCESS)
            {
                IC_CORE_WARN("vkQueueWaitIdle failed with result {}", static_cast<int>(result));
            }
        }
    }

    queue_family_indices queue_manager::findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        queue_family_indices indices;

        if (device == VK_NULL_HANDLE)
        {
            IC_CORE_ERROR("Invalid physical device");
            return indices;
        }

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        if (queueFamilyCount == 0)
        {
            IC_CORE_ERROR("No queue families found");
            return indices;
        }

        IC_CORE_TRACE("Queue Family Count: {}", queueFamilyCount);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        std::optional<uint32_t> dedicatedTransferFamily;
        std::optional<uint32_t> dedicatedComputeFamily;

        for (uint32_t i = 0; i < queueFamilyCount; ++i)
        {
            const auto& queueFamily = queueFamilies[i];

            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                if (!indices.graphicsFamily.has_value())
                {
                    IC_CORE_TRACE("Graphics Queue Found at family {}", i);
                    indices.graphicsFamily = i;
                }
            }

            if ((queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) && !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT))
            {
                if (!dedicatedComputeFamily.has_value())
                {
                    IC_CORE_TRACE("Dedicated Compute Queue Found at family {}", i);
                    dedicatedComputeFamily = i;
                }
            }
            else if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                if (!indices.computeFamily.has_value())
                {
                    IC_CORE_TRACE("Compute Queue Found at family {}", i);
                    indices.computeFamily = i;
                }
            }

            if ((queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) &&
                !(queueFamily.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)))
            {
                if (!dedicatedTransferFamily.has_value())
                {
                    IC_CORE_TRACE("Dedicated Transfer Queue Found at family {}", i);
                    dedicatedTransferFamily = i;
                }
            }

            if (surface != VK_NULL_HANDLE)
            {
                VkBool32 presentSupport = false;
                VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
                if (result == VK_SUCCESS && presentSupport)
                {
                    if (!indices.presentFamily.has_value())
                    {
                        IC_CORE_TRACE("Present Queue Found at family {}", i);
                        indices.presentFamily = i;
                    }
                }
                else if (result != VK_SUCCESS)
                {
                    IC_CORE_WARN("Failed to check present support for queue family {}: {}", i,
                                 static_cast<int>(result));
                }
            }
        }

        if (dedicatedComputeFamily.has_value())
        {
            indices.computeFamily = dedicatedComputeFamily;
            IC_CORE_TRACE("Using dedicated compute queue family {}", dedicatedComputeFamily.value());
        }

        if (dedicatedTransferFamily.has_value())
        {
            indices.transferFamily = dedicatedTransferFamily;
            IC_CORE_TRACE("Using dedicated transfer queue family {}", dedicatedTransferFamily.value());
        }

        if (!indices.transferFamily.has_value() && indices.graphicsFamily.has_value())
        {
            IC_CORE_TRACE("Using graphics family {} for transfer operations", indices.graphicsFamily.value());
            indices.transferFamily = indices.graphicsFamily;
        }

        if (!indices.computeFamily.has_value() && indices.graphicsFamily.has_value())
        {
            IC_CORE_TRACE("Using graphics family {} for compute operations", indices.graphicsFamily.value());
            indices.computeFamily = indices.graphicsFamily;
        }

        if (indices.isComplete())
        {
            IC_CORE_INFO("Queue family configuration:");
            IC_CORE_INFO("  Graphics: {}", indices.graphicsFamily.value());
            IC_CORE_INFO("  Present:  {}", indices.presentFamily.value());
            if (indices.computeFamily.has_value())
                IC_CORE_INFO("  Compute:  {}", indices.computeFamily.value());
            if (indices.transferFamily.has_value())
                IC_CORE_INFO("  Transfer: {}", indices.transferFamily.value());
        }
        else
        {
            IC_CORE_ERROR("Failed to find complete queue family configuration");
        }

        return indices;
    }
} // namespace ic