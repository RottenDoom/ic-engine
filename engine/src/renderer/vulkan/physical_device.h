#ifndef PHYSICAL_DEVICE_H
#define PHYSICAL_DEVICE_H

#include "defines.h"
#include "surface.h"
#include "device.h"

namespace ic
{
/** @brief DeviceInfo class contains the the information for each physical device handle such as features,
 * properties, queues and swapchain support. This is the basis for comparision between the GPU handles. */
struct DeviceInfo
{

        VkPhysicalDevice handle;

        // features and properties
        VkPhysicalDeviceProperties properties{};
        VkPhysicalDeviceFeatures features{};
        VkPhysicalDeviceMemoryProperties memoryProperties{};
        std::vector<VkExtensionProperties> availableExtensions;

        std::vector<QueueFamily> queues;
        int32_t graphicsQueueFamily = -1;
        int32_t computeQueueFamily  = -1;
        int32_t presentQueueFamily  = -1;
        int32_t transferQueueFamily = -1;

        SwapChainSupportDetails swapChainSupportDetails{};

        std::string label;

        bool supportsExtension(const char *name) const
        {
                for (const auto &e : availableExtensions)
                        if (std::strcmp(e.extensionName, name) == 0)
                                return true;
                return false;
        }

        bool supportsSwapchain() const
        {
                return swapChainSupportDetails.surfaceSupported && !swapChainSupportDetails.formats.empty() &&
                       !swapChainSupportDetails.presentModes.empty();
        }

        bool hasGraphicsAndCompute() const { return graphicsQueueFamily >= 0 && computeQueueFamily >= 0; }
};

/** @brief SelectionConfig is a struct for setting up the requiremnets for selecting a GPU handle. It sets up a
 * score based on the queues and properties and swapchain supports based on a score. Even though a score is does
 * not speak of the true differences between the GPU handles. For now I am just going to use this as the basis
 * later on manual selection would be the go. */
struct SelectionConfig
{
        uint32_t minVulkanVersion = VK_API_VERSION_1_0;
        std::vector<const char *> requiredExtensions;
        VkPhysicalDeviceFeatures requiredFeatures{};

        std::function<float(const DeviceInfo &)> score = [](const DeviceInfo &d) -> float
        {
                if (!d.hasGraphicsAndCompute() || !d.supportsSwapchain())
                        return -std::numeric_limits<float>::infinity();  // TODO: Use my own infinity

                // Prefer discrete, then VRAM, then API version
                float s = 0.0f;
                switch (d.properties.deviceType)
                {
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                        s += 1000.0f;
                        break;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                        s += 100.0f;
                        break;
                default:
                        break;
                }

                // Approx device-local heap size
                VkDeviceSize vram = 0;
                for (uint32_t i = 0; i < d.memoryProperties.memoryHeapCount; ++i)
                        if (d.memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
                                vram = std::max(vram, d.memoryProperties.memoryHeaps[i].size);
                s += static_cast<float>(vram / (1024.0 * 1024.0 * 1024.0));  // GB

                s += static_cast<float>(VK_API_VERSION_MAJOR(d.properties.apiVersion)) * 10.0f;
                return s;
        };
};

/** @brief Physical Device class is just wrapper for device selection. This class can be later extended to use
 * with UI configs for manual selection for devices */
class PhysicalDevice
{
private:
        VkSurfaceKHR m_surface            = VK_NULL_HANDLE;
        VkInstance m_instance             = VK_NULL_HANDLE;
        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;

        std::vector<DeviceInfo> m_devices;
        int m_selectedIndex = -1;

private:
        DeviceInfo buildInfo(VkPhysicalDevice handle) const;
        static bool meetsRequirements(const DeviceInfo &deviceInfo, const SelectionConfig &config);

public:
        PhysicalDevice(VkInstance &instance, const VkSurfaceKHR &surface);
        ~PhysicalDevice() = default;

        /** @brief These functions are for device selection.For future for UI GPU selections this implementation
         * might help */
        const std::vector<DeviceInfo> &enumerate();
        bool select(const SelectionConfig &config);
        bool selectByIndex(uint32_t index);
        bool selectByPredicate(const std::function<bool(const DeviceInfo &)> &pred);

        VkPhysicalDevice get() const { return m_physicalDevice; }
        int selectedIndex() const { return m_selectedIndex; }
        const std::vector<DeviceInfo> &devices() const { return m_devices; }
};

}  // namespace ic

#endif