#include "physical_device.h"
#include "context.h"
#include "defines.h"
#include "queue_manager.h"

// TODO: implement swapchain selection

namespace ic
{
    bool physical_device::select(VkInstance& instance, VkSurfaceKHR& surface)
    {
        if (!instance || !surface)
        {
            IC_CORE_ERROR("Invalid instance or surface for device selection");
            return false;
        }

        // enumerate devices
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        IC_CORE_FATAL_IF(deviceCount == 0, "Failed to find GPUs with Vulkan support");

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        // cache all device information once per device
        std::vector<device_info> deviceInfos;
        deviceInfos.reserve(deviceCount);

        IC_CORE_INFO("Found {} physical device(s)", deviceCount);

        for (const auto& device : devices)
        {
            device_info& info = deviceInfos.emplace_back();
            info.device = device;
            info.queryDeviceInfo(surface);

            IC_CORE_TRACE("Cached info for device: {}", info.properties.deviceName);
        }

        std::multimap<uint32_t, const device_info*> candidates;

        for (auto& deviceInfo : deviceInfos)
        {
            uint32_t score = rateDeviceSuitability(deviceInfo, m_requirements);
            if (score > 0)
            {
                candidates.insert(std::make_pair(score, &deviceInfo));
                IC_CORE_TRACE("Device '{}' scored: {}", deviceInfo.properties.deviceName, score);
            }
            else
            {
                IC_CORE_TRACE("Device '{}' failed suitability test", deviceInfo.properties.deviceName);
            }
        }

        if (candidates.empty())
        {
            IC_CORE_ERROR("Failed to find a suitable GPU!");
            return false;
        }

        const device_info* bestDevice = candidates.rbegin()->second;
        m_deviceInfo = *bestDevice; // Copy the cached information

        logDeviceInfo();

        IC_CORE_INFO("Selected physical device: {} (Score: {})", m_deviceInfo.properties.deviceName,
                     candidates.rbegin()->first);
        return true;
    }

    bool physical_device::isDeviceSuitable(device_info& deviceInfo, device_requirements& requirements)
    {
        // check queue families
        queue_family_indices indices =
            queue_manager::findQueueFamilies(deviceInfo.device, vulkan_context::get()->getSurface()->get());
        bool queueFamiliesComplete = indices.isComplete();
        if (queueFamiliesComplete)
        {
            IC_CORE_INFO("All required queue families found");
        }
        else
        {
            IC_CORE_WARN("Missing required queue families");
            if (!indices.graphicsFamily.has_value())
                IC_CORE_WARN("    - Missing graphics queue family");
            if (!indices.presentFamily.has_value())
                IC_CORE_WARN("    - Missing present queue family");
            if (!indices.computeFamily.has_value())
                IC_CORE_WARN("    - Missing compute queue family");
            if (!indices.transferFamily.has_value())
                IC_CORE_WARN("    - Missing transfer queue family");
        }

        // check extensions
        bool extensionsSupported = checkExtensionSupport(deviceInfo.device);
        bool swapChainAdequate = false;
        if (extensionsSupported)
        {
            IC_CORE_INFO("All required extensions supported");
            swap_chain_support_details swapChainSupport = vulkan_context::get()->getSwapChain()->querySupport(
                deviceInfo.device, vulkan_context::get()->getSurface()->get());
            swapChainAdequate = swapChainSupport.isAdequate();
        }

        IC_CORE_INFO("Supported features:");
        IC_CORE_INFO(" - samplerAnisotropy: {}", deviceInfo.features.samplerAnisotropy);
        IC_CORE_INFO(" - fillModeNonSolid: {}", deviceInfo.features.fillModeNonSolid);

        // check for all features
        IC_CORE_INFO("Required features:");
        IC_CORE_INFO(" - samplerAnisotropy: {}", m_requirements.requiredFeatures.samplerAnisotropy);
        IC_CORE_INFO(" - fillModeNonSolid: {}", m_requirements.requiredFeatures.fillModeNonSolid);

        bool featuresSupported = true;
        if (m_requirements.requiredFeatures.samplerAnisotropy && !deviceInfo.features.samplerAnisotropy)
        {
            featuresSupported = false;
        }
        if (m_requirements.requiredFeatures.fillModeNonSolid && !deviceInfo.features.fillModeNonSolid)
        {
            featuresSupported = false;
        }

        // more feature checks ....
        if (!extensionsSupported)
            IC_CORE_WARN(" -> Missing required extensions");
        if (!swapChainAdequate)
            IC_CORE_WARN(" -> Swapchain support inadequate");
        if (!featuresSupported)
            IC_CORE_WARN(" -> Required features not supported");

        return indices.isComplete() && extensionsSupported && swapChainAdequate && featuresSupported;
    }

    const queue_family_indices physical_device::getQueueFamilyIndices() const
    {
        if (m_deviceInfo.device != VK_NULL_HANDLE)
        {
            return queue_manager::findQueueFamilies(m_deviceInfo.device, vulkan_context::get()->getSurface()->get());
        }
        return queue_family_indices();
    }

    bool physical_device::checkExtensionSupport(VkPhysicalDevice device)
    {
        // extensions enumeration
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions;
        for (const auto& ext : m_requirements.requiredExtensions)
        {
            requiredExtensions.insert(std::string(ext));
        }

        for (const auto& extension : availableExtensions)
        {
            requiredExtensions.erase(extension.extensionName);
        }

        if (!requiredExtensions.empty())
        {
            IC_CORE_WARN("Missing extensions:");
            for (const auto& ext : requiredExtensions)
            {
                IC_CORE_WARN(" -> {}", ext);
            }
        }

        return requiredExtensions.empty();
    }

    uint32_t physical_device::rateDeviceSuitability(device_info& deviceInfo, device_requirements& requirements)
    {
        if (!isDeviceSuitable(deviceInfo, requirements))
        {
            return 0;
        }

        const auto& properties = deviceInfo.properties;
        const auto& features = deviceInfo.features;
        const auto& indices =
            queue_manager::findQueueFamilies(deviceInfo.device, vulkan_context::get()->getSurface()->get());

        int score = 0;

        // Discrete GPUs have a significant performance advantage
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            score += 1000;
            IC_CORE_INFO("  + 1000 points (Discrete GPU)");
        }
        else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
        {
            score += 100;
            IC_CORE_INFO("  + 100 points (Integrated GPU)");
        }

        // Maximum possible size of textures affects graphics quality
        // Maximum possible size of textures affects graphics quality
        int textureScore = static_cast<int>(properties.limits.maxImageDimension2D / 1000);
        score += textureScore;
        IC_CORE_INFO("  + {} points (Max texture size: {})", textureScore, properties.limits.maxImageDimension2D);

        // Prefer devices with more memory
        uint64_t totalMemory = 0;
        vkGetPhysicalDeviceMemoryProperties(deviceInfo.device, &deviceInfo.memoryProperties);

        for (uint32_t i = 0; i < deviceInfo.memoryProperties.memoryHeapCount; i++)
        {
            if (deviceInfo.memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
            {
                totalMemory += deviceInfo.memoryProperties.memoryHeaps[i].size;
            }
        }
        uint32_t memoryScore = static_cast<int>(totalMemory / (1024 * 1024 * 1024)); // score based on GB
        score += memoryScore;
        IC_CORE_INFO("  + {} points (Device memory: {:.2f} GB)", memoryScore, totalMemory / (1024.0 * 1024.0 * 1024.0));

        // Check queue family uniqueness (dedicated queues are better)
        std::set<uint32_t> uniqueQueueFamilies = indices.getUniqueQueueFamilies();
        score += static_cast<int>(uniqueQueueFamilies.size()) * 100;
        IC_CORE_INFO("   + {} * 100 points for each unique queue", static_cast<int>(uniqueQueueFamilies.size()));

        return score;
    }

    void physical_device::logDeviceInfo() const
    {
        const auto& properties = m_deviceInfo.properties;
        const auto& memoryProperties = m_deviceInfo.memoryProperties;
        const auto& indices =
            queue_manager::findQueueFamilies(m_deviceInfo.device, vulkan_context::get()->getSurface()->get());

        IC_CORE_INFO("Physical Device Info:");
        IC_CORE_INFO("  Name: {}", properties.deviceName);
        IC_CORE_INFO("  Type: {}",
                     properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? "Discrete GPU" : "Other");
        IC_CORE_INFO("  API Version: {}.{}.{}", VK_VERSION_MAJOR(properties.apiVersion),
                     VK_VERSION_MINOR(properties.apiVersion), VK_VERSION_PATCH(properties.apiVersion));
        IC_CORE_INFO("  Driver Version: {}", properties.driverVersion);
        IC_CORE_INFO("  Vendor ID: 0x{:X}", properties.vendorID);
        IC_CORE_INFO("  Device ID: 0x{:X}", properties.deviceID);

        // Log memory information
        uint64_t totalVRAM = 0;
        for (uint32_t i = 0; i < memoryProperties.memoryHeapCount; i++)
        {
            if (memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
            {
                totalVRAM += memoryProperties.memoryHeaps[i].size;
            }
        }
        IC_CORE_INFO("  VRAM: {:.1f} GB", static_cast<double>(totalVRAM) / (1024.0 * 1024.0 * 1024.0));

        // Log queue families
        IC_CORE_INFO("  Queue Families:");
        IC_CORE_INFO("    Graphics: {}", indices.graphicsFamily.value_or(UINT32_MAX));
        IC_CORE_INFO("    Present: {}", indices.presentFamily.value_or(UINT32_MAX));
        if (indices.computeFamily.has_value())
        {
            IC_CORE_INFO("    Compute: {}", indices.computeFamily.value());
        }
        if (indices.transferFamily.has_value())
        {
            IC_CORE_INFO("    Transfer: {}", indices.transferFamily.value());
        }

        IC_CORE_INFO("  Supported Extensions: {}", m_deviceInfo.availableExtensions.size());
    }
} // namespace ic
