#include "physical_device.h"
#include "context.h"
#include "defines.h"

// TODO: implement swapchain selection

namespace ic
{
        DeviceInfo PhysicalDevice::buildInfo(VkPhysicalDevice handle) const
        {
                DeviceInfo out{};
                out.handle = handle;

                // core props/features/memory
                vkGetPhysicalDeviceProperties(handle, &out.properties);
                vkGetPhysicalDeviceFeatures(handle, &out.features);
                vkGetPhysicalDeviceMemoryProperties(handle, &out.memoryProperties);

                uint32_t qcount = 0;
                vkGetPhysicalDeviceQueueFamilyProperties(handle, &qcount, nullptr);
                std::vector<VkQueueFamilyProperties> qprops(qcount);
                vkGetPhysicalDeviceQueueFamilyProperties(handle, &qcount, qprops.data());

                IC_CORE_TRACE("{0} queues found on the GPU device handle {1}", qcount, out.properties.deviceName);

                out.queues.reserve(qcount);
                for (uint32_t i = 0; i < qcount; ++i)
                {
                        QueueFamily q{};
                        q.index = i;
                        q.flags = qprops[i].queueFlags;
                        q.count = qprops[i].queueCount;
                        out.queues.push_back(q);
                }
                // find G+C
                for (const auto& q : out.queues)
                {
                        if ((q.flags & VK_QUEUE_GRAPHICS_BIT) && out.graphicsQueueFamily < 0)
                                out.graphicsQueueFamily = static_cast<int>(q.index);
                        if ((q.flags & VK_QUEUE_COMPUTE_BIT) && out.computeQueueFamily < 0)
                                out.computeQueueFamily = static_cast<int>(q.index);
                        if ((q.flags & VK_QUEUE_TRANSFER_BIT) && out.transferQueueFamily < 0)
                                out.transferQueueFamily = static_cast<int>(q.index);
                }
                // present support (surface-dependent)
                out.presentQueueFamily = -1;
                if (m_surface != VK_NULL_HANDLE)
                {
                        for (const auto& q : out.queues)
                        {
                                VkBool32 supported = VK_FALSE;
                                vkGetPhysicalDeviceSurfaceSupportKHR(handle, q.index, m_surface, &supported);
                                if (supported && out.presentQueueFamily < 0)
                                        out.presentQueueFamily = static_cast<int>(q.index);
                        }
                        out.swapChainSupportDetails.surfaceSupported = out.presentQueueFamily >= 0;
                        if (out.swapChainSupportDetails.surfaceSupported)
                        {
                                uint32_t formatCount = 0;
                                vkGetPhysicalDeviceSurfaceFormatsKHR(handle, m_surface, &formatCount, nullptr);
                                out.swapChainSupportDetails.formats.resize(formatCount);
                                if (formatCount)
                                        vkGetPhysicalDeviceSurfaceFormatsKHR(handle,
                                                                             m_surface,
                                                                             &formatCount,
                                                                             out.swapChainSupportDetails.formats.data());

                                uint32_t presentModes = 0;
                                vkGetPhysicalDeviceSurfacePresentModesKHR(handle, m_surface, &presentModes, nullptr);
                                out.swapChainSupportDetails.presentModes.resize(presentModes);
                                if (presentModes)
                                        vkGetPhysicalDeviceSurfacePresentModesKHR(
                                            handle,
                                            m_surface,
                                            &presentModes,
                                            out.swapChainSupportDetails.presentModes.data());

                                vkGetPhysicalDeviceSurfaceCapabilitiesKHR(handle,
                                                                          m_surface,
                                                                          &out.swapChainSupportDetails.capabilities);
                        }
                }

                // extensions
                uint32_t extCount = 0;
                vkEnumerateDeviceExtensionProperties(handle, nullptr, &extCount, nullptr);
                out.availableExtensions.resize(extCount);
                if (extCount)
                        vkEnumerateDeviceExtensionProperties(handle, nullptr, &extCount, out.availableExtensions.data());
                for (size_t i = 0; i < extCount; i++)
                {
                        IC_CORE_TRACE("    {0}. {1}", i + 1, out.availableExtensions[i].extensionName);
                }
                IC_CORE_TRACE("{0} extensions found on the GPU handle {1}", extCount, out.properties.deviceName);

                // label
                const char* typeStr = "Other";
                switch (out.properties.deviceType)
                {
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                        typeStr = "Discrete";
                        break;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                        typeStr = "Integrated";
                        break;
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                        typeStr = "Virtual";
                        break;
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                        typeStr = "CPU";
                        break;
                default:
                        break;
                }
                out.label = std::string(out.properties.deviceName) + " (" + typeStr + ")";
                IC_CORE_TRACE("GPU handle labeled {0} is set up for comparision", out.label);
                return out;
        }

        bool PhysicalDevice::meetsRequirements(const DeviceInfo& deviceInfo, const SelectionConfig& config)
        {
                if (deviceInfo.properties.apiVersion < config.minVulkanVersion)
                        return false;

                // required extensions
                for (const char* ext : config.requiredExtensions)
                        if (!deviceInfo.supportsExtension(ext))
                                return false;

                // required features TODO(make a macro for checking and enabling extensions future proofing)
                const auto& req = config.requiredFeatures;
                // Example: if you set req.samplerAnisotropy = VK_TRUE, we enforce it
                if (req.samplerAnisotropy && !deviceInfo.features.samplerAnisotropy)
                        return false;
                if (req.geometryShader && !deviceInfo.features.geometryShader)
                        return false;
                if (req.tessellationShader && !deviceInfo.features.tessellationShader)
                        return false;
                // Extend with other fields you actually set in req...

                return true;
        }

        PhysicalDevice::PhysicalDevice(VkInstance& instance, const VkSurfaceKHR& surface)
            : m_instance(instance), m_surface(surface)
        {
        }

        const std::vector<DeviceInfo>& PhysicalDevice::enumerate()
        {
                if (!m_devices.empty())
                        return m_devices;
                uint32_t count = 0;
                vkEnumeratePhysicalDevices(m_instance, &count, nullptr);
                std::vector<VkPhysicalDevice> phys(count);
                vkEnumeratePhysicalDevices(m_instance, &count, phys.data());

                m_devices.reserve(count);
                for (auto h : phys)
                        m_devices.push_back(buildInfo(h));
                return m_devices;
        }

        bool PhysicalDevice::select(const SelectionConfig& config)
        {
                const auto& devices = enumerate();
                float bestScore     = -std::numeric_limits<float>::infinity();
                int bestIdx         = -1;
                for (int i = 0; i < static_cast<int>(devices.size()); ++i)
                {
                        // Check if the device meets all requirements
                        if (!meetsRequirements(devices[i], config))
                                continue;
                        // If device meets requirements then check the score of device if not zero
                        float score = config.score ? config.score(devices[i]) : 0.0f;
                        if (score > bestScore)
                        {
                                bestScore = score;
                                bestIdx   = i;
                        }
                }
                if (bestIdx >= 0)
                {
                        m_physicalDevice = devices[bestIdx].handle;
                        m_selectedIndex  = bestIdx;
                        IC_CORE_TRACE("{0} GPU handle was selected based on the requirements. ",
                                      devices[bestIdx].label);
                        return true;
                }
                return false;
        }

        bool PhysicalDevice::selectByIndex(uint32_t index)
        {
                const auto& devices = enumerate();
                if (index >= devices.size())
                        return false;
                m_physicalDevice = devices[index].handle;
                m_selectedIndex  = static_cast<int>(index);
                return true;
        }

        bool PhysicalDevice::selectByPredicate(const std::function<bool(const DeviceInfo&)>& pred)
        {
                const auto& devices = enumerate();
                for (int i = 0; i < static_cast<int>(devices.size()); ++i)
                {
                        if (pred(devices[i]))
                        {
                                m_physicalDevice = devices[i].handle;
                                m_selectedIndex  = i;
                                return true;
                        }
                }
                return false;
        }

}  // namespace ic
