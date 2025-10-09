#pragma once
#include "queue_manager.h"
#include "surface.h"

namespace ic
{
        class vulkan_context;

        struct device_info
        {
                VkPhysicalDevice device = VK_NULL_HANDLE;
                VkPhysicalDeviceProperties properties{};
                VkPhysicalDeviceFeatures features{};
                VkPhysicalDeviceMemoryProperties memoryProperties{};
                std::vector<VkExtensionProperties> availableExtensions;

                // Cache all device info once
                void queryDeviceInfo(VkSurfaceKHR surface)
                {
                        vkGetPhysicalDeviceProperties(device, &properties);
                        vkGetPhysicalDeviceFeatures(device, &features);
                        vkGetPhysicalDeviceMemoryProperties(device, &memoryProperties);

                        // Query available extensions
                        uint32_t extensionCount;
                        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
                        availableExtensions.resize(extensionCount);
                        vkEnumerateDeviceExtensionProperties(device,
                                                             nullptr,
                                                             &extensionCount,
                                                             availableExtensions.data());
                }
        };

        struct device_requirements
        {
                std::vector<const char*> requiredExtensions;  // Use vector for dynamic extensions

                VkPhysicalDeviceFeatures requiredFeatures{};
                bool requiresDedicatedGPU = false;
                uint32_t minVulkanVersion = VK_API_VERSION_1_0;

                device_requirements()
                {
                        requiredExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

                        requiredFeatures.samplerAnisotropy = VK_TRUE;
                        requiredFeatures.fillModeNonSolid  = VK_TRUE;
                        // TODO - add more requirements and extensions
                }

                void addExtension(const char* extension) { requiredExtensions.push_back(extension); }
        };

        class physical_device
        {
        private:
                device_info m_deviceInfo;
                device_requirements m_requirements;
                VkInstance m_instance;
                VkSurfaceKHR m_surface;

        public:
                physical_device(VkInstance& instance, const VkSurfaceKHR& surface);
                ~physical_device() = default;

                bool select(VkInstance& instance, VkSurfaceKHR& surface);

                VkPhysicalDevice get() const { return m_deviceInfo.device; }
                operator VkPhysicalDevice() const { return m_deviceInfo.device; }

                const VkPhysicalDeviceProperties& getProperties() const { return m_deviceInfo.properties; }
                const VkPhysicalDeviceFeatures& getFeatures() const { return m_deviceInfo.features; }
                const VkPhysicalDeviceMemoryProperties& getMemoryProperties() const
                {
                        return m_deviceInfo.memoryProperties;
                }
                const device_requirements& getRequirements() const { return m_requirements; }

                const queue_family_indices getQueueFamilyIndices() const;

        private:
                // TODO: rewrite these functions
                bool isDeviceSuitable(device_info& deviceInfo, device_requirements& requirements);
                bool checkExtensionSupport(VkPhysicalDevice device);
                uint32_t rateDeviceSuitability(device_info& deviceInfo, device_requirements& requirements);
                void logDeviceInfo() const;  // TODO write this function.
        };

}  // namespace ic