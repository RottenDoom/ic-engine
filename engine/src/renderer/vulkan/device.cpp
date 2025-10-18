#include "device.h"

namespace ic
{
        vkdevice::vkdevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) : surface(surface)
        {
                IC_CORE_ASSERT(physicalDevice, "Physical Device does not exist");
                this->physicalDevice = physicalDevice;

                vkGetPhysicalDeviceProperties(physicalDevice, &properties);
                vkGetPhysicalDeviceFeatures(physicalDevice, &features);
                vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

                uint32_t queueFamilyCount;
                vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
                IC_CORE_ASSERT(queueFamilyCount > 0, "No queues found!");
                queueFamilyProps.resize(queueFamilyCount);
                vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProps.data());

                uint32_t extCount = 0;
                vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, nullptr);
                IC_CORE_ASSERT(extCount > 0, "No extensions Found!");
                if (extCount > 0)
                {
                        std::vector<VkExtensionProperties> extensions(extCount);
                        if (vkEnumerateDeviceExtensionProperties(
                                physicalDevice, nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
                        {
                                for (auto& ext : extensions)
                                {
                                        supportedExtensions.push_back(ext.extensionName);
                                }
                        }
                }
        }

        vkdevice::~vkdevice() {}

        void vkdevice::destroy()
        {
                if (cmdPool)
                {
                        vkDestroyCommandPool(logicalDevice, cmdPool, nullptr);
                }
                if (logicalDevice)
                {
                        vkDestroyDevice(logicalDevice, nullptr);
                }
        }

        uint32_t vkdevice::getMemoryType(uint32_t typeBits,
                                         VkMemoryPropertyFlags properties,
                                         VkBool32* memTypeFound) const
        {
                for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
                {
                        if ((typeBits & 1) == 1)
                        {
                                if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
                                {
                                        if (memTypeFound)
                                        {
                                                *memTypeFound = true;
                                        }
                                        return i;
                                }
                        }
                        typeBits >>= 1;
                }

                if (memTypeFound)
                {
                        *memTypeFound = false;
                        return 0;
                }
                else
                {
                        IC_CORE_ERROR("Could Not find matching memory Type!");
                        return 0;
                }
        }

        uint32_t vkdevice::getQueueFamilyIndex(VkQueueFlags queueFlags) const
        {
                // dedicated queue for compute
                if ((queueFlags & VK_QUEUE_COMPUTE_BIT) == queueFlags)
                {
                        for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProps.size()); i++)
                        {
                                if ((queueFamilyProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
                                    ((queueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
                                {
                                        return i;
                                }
                        }
                }

                // dedicated queue for transfer
                if ((queueFlags & VK_QUEUE_TRANSFER_BIT) == queueFlags)
                {
                        for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProps.size()); i++)
                        {
                                if ((queueFamilyProps[i].queueFlags & VK_QUEUE_TRANSFER_BIT) &&
                                    ((queueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) &&
                                    ((queueFamilyProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
                                {
                                        return i;
                                }
                        }
                }

                // return the first one
                for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProps.size()); i++)
                {
                        if ((queueFamilyProps[i].queueFlags & queueFlags) == queueFlags)
                        {
                                return i;
                        }
                }

                IC_CORE_ERROR("Could not find a matching queue family index!");
                return 0;
        }

        VkResult vkdevice::createLogicalDevice(VkPhysicalDeviceFeatures enabledFeatures,
                                               std::vector<const char*> enabledExtensions,
                                               void* pNextChain,
                                               bool useSwapChain,
                                               VkQueueFlags requestedQueueTypes)
        {
                std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};
                const float defaultQueuePriority(0.0f);

                // Graphics queue
                if (requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT)
                {
                        queues.graphics.index = getQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
                        queues.graphics.flags = VK_QUEUE_GRAPHICS_BIT;
                        queues.graphics.count = 1;

                        VkDeviceQueueCreateInfo queueInfo{.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                                          .queueFamilyIndex = queues.graphics.index,
                                                          .queueCount       = 1,
                                                          .pQueuePriorities = &defaultQueuePriority};
                        queueCreateInfos.push_back(queueInfo);
                }
                else
                {
                        queues.graphics.index = UINT32_MAX;
                }

                // Dedicated compute queue
                if (requestedQueueTypes & VK_QUEUE_COMPUTE_BIT)
                {
                        queues.compute.index = getQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT);
                        queues.compute.flags = VK_QUEUE_COMPUTE_BIT;
                        queues.compute.count = 1;

                        if (queues.compute.index != queues.graphics.index)
                        {
                                VkDeviceQueueCreateInfo queueInfo{.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                                                  .queueFamilyIndex = queues.compute.index,
                                                                  .queueCount       = 1,
                                                                  .pQueuePriorities = &defaultQueuePriority};
                                queueCreateInfos.push_back(queueInfo);
                        }
                }
                else
                {
                        queues.compute.index = queues.graphics.index;
                        queues.compute.flags = queues.graphics.flags;
                        queues.compute.count = 0;  // Sharing graphics queue
                }

                // Dedicated transfer queue
                if (requestedQueueTypes & VK_QUEUE_TRANSFER_BIT)
                {
                        queues.transfer.index = getQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT);
                        queues.transfer.flags = VK_QUEUE_TRANSFER_BIT;
                        queues.transfer.count = 1;

                        if ((queues.transfer.index != queues.graphics.index) &&
                            (queues.transfer.index != queues.compute.index))
                        {
                                VkDeviceQueueCreateInfo queueInfo{.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                                                  .queueFamilyIndex = queues.transfer.index,
                                                                  .queueCount       = 1,
                                                                  .pQueuePriorities = &defaultQueuePriority};
                                queueCreateInfos.push_back(queueInfo);
                        }
                }
                else
                {
                        queues.transfer.index = queues.graphics.index;
                        queues.transfer.flags = queues.graphics.flags;
                        queues.transfer.count = 0;  // Sharing graphics queue
                }

                // Handle present queue if surface is available
                if (surface != VK_NULL_HANDLE)
                {
                        // Find queue family that supports presentation
                        for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProps.size()); i++)
                        {
                                VkBool32 presentSupport = false;
                                vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

                                if (presentSupport)
                                {
                                        queues.present.index = i;
                                        queues.present.flags = 0;  // Present doesn't have a standard flag
                                        queues.present.count = 1;

                                        // Only create a new queue if present family differs from existing ones
                                        bool needsNewQueue = true;
                                        for (const auto& qci : queueCreateInfos)
                                        {
                                                if (qci.queueFamilyIndex == i)
                                                {
                                                        needsNewQueue = false;
                                                        break;
                                                }
                                        }

                                        if (needsNewQueue)
                                        {
                                                VkDeviceQueueCreateInfo queueInfo{
                                                    .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                                    .queueFamilyIndex = i,
                                                    .queueCount       = 1,
                                                    .pQueuePriorities = &defaultQueuePriority};
                                                queueCreateInfos.push_back(queueInfo);
                                        }
                                        break;
                                }
                        }
                }

                // Create the logical device representation
                std::vector<const char*> deviceExtensions(enabledExtensions);
                if (useSwapChain)
                {
                        deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
                }

                VkDeviceCreateInfo deviceCreateInfo{.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                                                    .queueCreateInfoCount = static_cast<uint32_t>(
                                                        queueCreateInfos.size()),
                                                    .pQueueCreateInfos = queueCreateInfos.data(),
                                                    .pEnabledFeatures  = &enabledFeatures};

                // If a pNext(Chain) has been passed, add it to the device creation info
                VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{};
                if (pNextChain)
                {
                        physicalDeviceFeatures2.sType     = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
                        physicalDeviceFeatures2.features  = enabledFeatures;
                        physicalDeviceFeatures2.pNext     = pNextChain;
                        deviceCreateInfo.pEnabledFeatures = nullptr;
                        deviceCreateInfo.pNext            = &physicalDeviceFeatures2;
                }

                // Validate and set extensions
                if (deviceExtensions.size() > 0)
                {
                        for (const char* enabledExtension : deviceExtensions)
                        {
                                if (!extensionSupported(enabledExtension))
                                {
                                        IC_CORE_CRITICAL("Enabled device extension {} is not present at device "
                                                         "level\n",
                                                         enabledExtension);
                                }
                        }

                        deviceCreateInfo.enabledExtensionCount   = (uint32_t)deviceExtensions.size();
                        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
                }

                this->enabledFeatures = enabledFeatures;

                // Create the logical device
                VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &logicalDevice);
                if (result != VK_SUCCESS)
                {
                        IC_CORE_ERROR("Device Creation Failed");
                        return result;
                }

                // Retrieve queue handles
                if (queues.graphics.index != UINT32_MAX)
                {
                        vkGetDeviceQueue(logicalDevice, queues.graphics.index, 0, &queues.graphics.handle);
                }

                if (queues.compute.index != UINT32_MAX && queues.compute.count > 0)
                {
                        vkGetDeviceQueue(logicalDevice, queues.compute.index, 0, &queues.compute.handle);
                }
                else if (queues.compute.index == queues.graphics.index)
                {
                        // Reuse graphics queue handle
                        queues.compute.handle = queues.graphics.handle;
                }

                if (queues.transfer.index != UINT32_MAX && queues.transfer.count > 0)
                {
                        vkGetDeviceQueue(logicalDevice, queues.transfer.index, 0, &queues.transfer.handle);
                }
                else if (queues.transfer.index == queues.graphics.index)
                {
                        // Reuse graphics queue handle
                        queues.transfer.handle = queues.graphics.handle;
                }

                if (queues.present.index != UINT32_MAX)
                {
                        vkGetDeviceQueue(logicalDevice, queues.present.index, 0, &queues.present.handle);
                }

                // Create a default command pool for graphics command buffers
                if (queues.graphics.index != UINT32_MAX)
                {
                        cmdPool = createCommandPool(queues.graphics.index);
                }

                return result;
        }

        VkResult vkdevice::createBuffer(VkBufferUsageFlags usageFlags,
                                        VkMemoryPropertyFlags memoryPropertyFlags,
                                        VkDeviceSize size,
                                        VkBuffer* buffer,
                                        VkDeviceMemory* memory,
                                        void* data)
        {
                // Create the buffer handle
                VkBufferCreateInfo bufCreateInfo{};
                bufCreateInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                bufCreateInfo.usage       = usageFlags;
                bufCreateInfo.size        = size;
                bufCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                IC_CORE_ASSERT(vkCreateBuffer(logicalDevice, &bufCreateInfo, nullptr, buffer) == VK_SUCCESS,
                               "Failed to create Buffer!");

                // Create the memory backing up the buffer handle
                VkMemoryRequirements memReqs;
                VkMemoryAllocateInfo memAlloc{};
                memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                vkGetBufferMemoryRequirements(logicalDevice, *buffer, &memReqs);
                memAlloc.allocationSize = memReqs.size;
                // Find a memory type index that fits the properties of the buffer
                memAlloc.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
                // If the buffer has VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT set we also need to enable the
                // appropriate flag during allocation
                VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
                if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
                {
                        allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
                        allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
                        memAlloc.pNext       = &allocFlagsInfo;
                }
                IC_CORE_ASSERT(vkAllocateMemory(logicalDevice, &memAlloc, nullptr, memory) == VK_SUCCESS,
                               "Failed to allocate buffer memory!");

                // If a pointer to the buffer data has been passed, map the buffer and copy over the data
                if (data != nullptr)
                {
                        void* mapped;
                        IC_CORE_ASSERT(vkMapMemory(logicalDevice, *memory, 0, size, 0, &mapped) == VK_SUCCESS,
                                       "Failed to map memory");
                        memcpy(mapped, data, size);
                        // If host coherency hasn't been requested, do a manual flush to make writes visible
                        if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
                        {
                                VkMappedMemoryRange mappedRange{.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
                                                                .memory = *memory,
                                                                .size   = size};
                                vkFlushMappedMemoryRanges(logicalDevice, 1, &mappedRange);
                        }
                        vkUnmapMemory(logicalDevice, *memory);
                }

                // Attach the memory to the buffer object
                IC_CORE_ASSERT(vkBindBufferMemory(logicalDevice, *buffer, *memory, 0) != VK_SUCCESS,
                               "Couldn't Bind Memory");

                return VK_SUCCESS;
        }

        VkResult vkdevice::createBuffer(VkBufferUsageFlags usageFlags,
                                        VkMemoryPropertyFlags memoryPropertyFlags,
                                        buffer* buffer,
                                        VkDeviceSize size,
                                        void* data)
        {
                VkBufferCreateInfo bufCreateInfo{};
                bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                bufCreateInfo.usage = usageFlags;
                bufCreateInfo.size  = size;
                IC_CORE_ASSERT(vkCreateBuffer(logicalDevice, &bufCreateInfo, nullptr, &buffer->handle) == VK_SUCCESS,
                               "Failed to create buffer!");

                // Create the memory backing up the buffer handle
                VkMemoryRequirements memReqs;
                VkMemoryAllocateInfo memAlloc{};
                memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                vkGetBufferMemoryRequirements(logicalDevice, buffer->handle, &memReqs);
                memAlloc.allocationSize = memReqs.size;
                // Find a memory type index that fits the properties of the buffer
                memAlloc.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
                // If the buffer has VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT set we also need to enable the
                // appropriate flag during allocation
                VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
                if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
                {
                        allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
                        allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
                        memAlloc.pNext       = &allocFlagsInfo;
                }
                IC_CORE_ASSERT(vkAllocateMemory(logicalDevice, &memAlloc, nullptr, &buffer->memory) == VK_SUCCESS,
                               "Failed to allocate memory");

                buffer->alignment           = memReqs.alignment;
                buffer->size                = size;
                buffer->usageFlags          = usageFlags;
                buffer->memoryPropertyFlags = memoryPropertyFlags;

                // If a pointer to the buffer data has been passed, map the buffer and copy over the data
                if (data != nullptr)
                {
                        IC_CORE_ASSERT(buffer->map() == VK_SUCCESS, "Failed to map buffer");
                        memcpy(buffer->mapped, data, size);
                        if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
                                buffer->flush();

                        buffer->unmap();
                }

                // Initialize a default descriptor that covers the whole buffer size
                buffer->setupDescriptor();

                // Attach the memory to the buffer object
                return buffer->bind();
        }

        void vkdevice::copyBuffer(buffer* src, buffer* dst, VkQueue queue, VkBufferCopy* copyRegion)
        {
                IC_CORE_ASSERT(dst->size >= src->size, "Buffer size do not match!");
                IC_CORE_ASSERT(src->handle, "Source buffer does not exist");
                VkCommandBuffer copyCmd = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
                VkBufferCopy bufferCopy{};
                if (copyRegion == nullptr)
                {
                        bufferCopy.size = src->size;
                }
                else
                {
                        bufferCopy = *copyRegion;
                }

                vkCmdCopyBuffer(copyCmd, src->handle, dst->handle, 1, &bufferCopy);

                flushCommandBuffer(copyCmd, queue);
        }

        VkCommandPool vkdevice::createCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags)
        {
                VkCommandPoolCreateInfo cmdPoolInfo{.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                                                    .flags            = createFlags,
                                                    .queueFamilyIndex = queueFamilyIndex};
                VkCommandPool cmdPool;
                if (vkCreateCommandPool(logicalDevice, &cmdPoolInfo, nullptr, &cmdPool) != VK_SUCCESS)
                {
                        IC_CORE_ERROR("Failed to create command pool!");
                }
                return cmdPool;
        }

        VkCommandBuffer vkdevice::createCommandBuffer(VkCommandBufferLevel level, VkCommandPool pool, bool begin)
        {
                VkCommandBufferAllocateInfo allocInfo{};
                allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                allocInfo.commandPool        = cmdPool;
                allocInfo.level              = level;
                allocInfo.commandBufferCount = 1;
                VkCommandBuffer cmdBuffer;
                IC_CORE_ASSERT(vkAllocateCommandBuffers(logicalDevice, &allocInfo, &cmdBuffer) == VK_SUCCESS,
                               "Commmand Buffer Info allocation failed!");
                // if requested, also start recording for the new command buffer
                if (begin)
                {
                        VkCommandBufferBeginInfo cmdBufInfo{};
                        cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                        IC_CORE_ASSERT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo) == VK_SUCCESS,
                                       "Command Buffer Begin failed!");
                }
                return cmdBuffer;
        }

        VkCommandBuffer vkdevice::createCommandBuffer(VkCommandBufferLevel level, bool begin)
        {
                return createCommandBuffer(level, cmdPool, begin);
        }

        void vkdevice::flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool pool, bool free)
        {
                if (commandBuffer == VK_NULL_HANDLE)
                {
                        return;
                }

                IC_CORE_ASSERT(vkEndCommandBuffer(commandBuffer) == VK_SUCCESS, "End of Command Buffer Failed");

                VkSubmitInfo submitInfo{.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                                        .commandBufferCount = 1,
                                        .pCommandBuffers    = &commandBuffer};

                // Create fence to ensure that the command buffer has finished executing
                VkFenceCreateInfo fenceCreateInfo{};
                fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
                fenceCreateInfo.flags = 0;
                VkFence fence;
                IC_CORE_ASSERT(vkCreateFence(logicalDevice, &fenceCreateInfo, nullptr, &fence) == VK_SUCCESS,
                               "Couldn't create fence");

                // Submit to the queue
                IC_CORE_ASSERT(vkQueueSubmit(queue, 1, &submitInfo, fence) == VK_SUCCESS, "Queue Submition failed");

                // wait for the fence to signal that command buffer has finished executing
                IC_CORE_ASSERT(vkWaitForFences(logicalDevice, 1, &fence, VK_TRUE, UINT64_MAX) == VK_SUCCESS,
                               "Timed Out!");

                vkDestroyFence(logicalDevice, fence, nullptr);
                if (free)
                {
                        vkFreeCommandBuffers(logicalDevice, pool, 1, &commandBuffer);
                }
        }

        void vkdevice::flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free)
        {
                return flushCommandBuffer(commandBuffer, queue, cmdPool, free);
        }

        bool vkdevice::extensionSupported(std::string extension)
        {
                return (std::find(supportedExtensions.begin(), supportedExtensions.end(), extension) !=
                        supportedExtensions.end());
        }

        VkFormat vkdevice::getSupportedDepthFormat(bool checkSamplingSupport)
        {
                std::vector<VkFormat> depthFormats = {VK_FORMAT_D32_SFLOAT_S8_UINT,
                                                      VK_FORMAT_D32_SFLOAT,
                                                      VK_FORMAT_D24_UNORM_S8_UINT,
                                                      VK_FORMAT_D16_UNORM_S8_UINT,
                                                      VK_FORMAT_D16_UNORM};
                for (auto& format : depthFormats)
                {
                        VkFormatProperties formatProperties;
                        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);
                        // Format must support depth stencil attachment for optimal tiling
                        if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
                        {
                                if (checkSamplingSupport)
                                {
                                        if (!(formatProperties.optimalTilingFeatures &
                                              VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT))
                                        {
                                                continue;
                                        }
                                }
                                return format;
                        }
                }
                IC_CORE_ERROR("Could not find a matching depth format!");
                return VK_FORMAT_UNDEFINED;
        }

        SwapChainSupportDetails vkdevice::getSwapChainSupport(VkPhysicalDevice device)
        {
                SwapChainSupportDetails details;
                vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

                uint32_t formatCount;
                vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

                if (formatCount != 0)
                {
                        details.formats.resize(formatCount);
                        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
                }

                uint32_t presentModeCount;
                vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

                if (presentModeCount != 0)
                {
                        details.presentModes.resize(presentModeCount);
                        vkGetPhysicalDeviceSurfacePresentModesKHR(device,
                                                                  surface,
                                                                  &presentModeCount,
                                                                  details.presentModes.data());
                }
                return details;
        }

}  // namespace ic
