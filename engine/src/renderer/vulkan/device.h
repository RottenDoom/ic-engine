#pragma once
#include "defines.h"

#include "buffer.h"

namespace ic
{

        struct SwapChainSupportDetails
        {
                bool surfaceSupported = false;
                VkSurfaceCapabilitiesKHR capabilities;
                std::vector<VkSurfaceFormatKHR> formats;
                std::vector<VkPresentModeKHR> presentModes;
        };

        struct QueueFamily
        {
                uint32_t index     = UINT32_MAX;
                VkQueueFlags flags = 0;
                uint32_t count     = 0;
                VkQueue handle     = VK_NULL_HANDLE;
        };

        /** @brief vkdevice is a unified class for logical and physical device. The selection of physical device is done
         * through the PhysicalDevice class and this takes in those features.*/
        struct vkdevice
        {
                VkPhysicalDevice physicalDevice{VK_NULL_HANDLE};
                VkDevice logicalDevice{VK_NULL_HANDLE};
                VkPhysicalDeviceProperties properties{};
                VkPhysicalDeviceFeatures features{};
                VkPhysicalDeviceFeatures enabledFeatures{};
                VkPhysicalDeviceMemoryProperties memoryProperties{};
                std::vector<VkQueueFamilyProperties> queueFamilyProps{};
                std::vector<std::string> supportedExtensions{};
                VkCommandPool cmdPool{};
                VkSurfaceKHR surface = VK_NULL_HANDLE;

                struct
                {
                        QueueFamily graphics;
                        QueueFamily compute;
                        QueueFamily transfer;
                        QueueFamily present;
                } queues;

                explicit vkdevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
                ~vkdevice();
                void destroy();

                uint32_t getMemoryType(uint32_t typeBits,
                                       VkMemoryPropertyFlags properties,
                                       VkBool32* memTypeFound = nullptr) const;
                uint32_t getQueueFamilyIndex(VkQueueFlags queueFlags) const;
                VkResult createLogicalDevice(VkPhysicalDeviceFeatures enabledFeatures,
                                             std::vector<const char*> enabledExtensions,
                                             void* pNextChain,
                                             bool useSwapChain                = true,
                                             VkQueueFlags requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT |
                                                                                VK_QUEUE_COMPUTE_BIT);

                VkResult createBuffer(VkBufferUsageFlags usageFlags,
                                      VkMemoryPropertyFlags memoryPropertyFlags,
                                      VkDeviceSize size,
                                      VkBuffer* buffer,
                                      VkDeviceMemory* memory,
                                      void* data = nullptr);

                VkResult createBuffer(VkBufferUsageFlags usageFlags,
                                      VkMemoryPropertyFlags memoryPropertyFlags,
                                      buffer* buffer,
                                      VkDeviceSize size,
                                      void* data = nullptr);

                void copyBuffer(buffer* src, buffer* dst, VkQueue queue, VkBufferCopy* copyRegion = nullptr);

                VkCommandPool createCommandPool(
                    uint32_t queueFamilyIndex,
                    VkCommandPoolCreateFlags createFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

                VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level, VkCommandPool pool, bool begin = false);
                VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level, bool begin = false);
                void
                flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool pool, bool free = true);
                void flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free = true);

                bool extensionSupported(std::string extension);
                VkFormat getSupportedDepthFormat(bool checkSamplingSupport);
                SwapChainSupportDetails getSwapChainSupport(VkPhysicalDevice device);
        };
}  // namespace ic
