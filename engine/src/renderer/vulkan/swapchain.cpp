#include "swapchain.h"
#include "core/application.h"

namespace ic
{
        SwapChain::SwapChain(ic::vkdevice& deviceRef, VkExtent2D extent) : device(deviceRef), windowExtent(extent)
        {
                init();
        }

        SwapChain::SwapChain(ic::vkdevice& deviceRef, VkExtent2D extent, SwapChain* oldSwapChain)
            : device(deviceRef), windowExtent(extent), oldSwapChain(oldSwapChain)
        {
                init();
        }

        VkFormat SwapChain::findDepthFormat()
        {
                return device.getSupportedDepthFormat(true);  // TODO: check this out
        }

        void SwapChain::destroy(VkDevice& device, const VkAllocationCallbacks* alloc)
        {
                for (size_t i = 0; i < swapChainImageViews.size(); i++)
                {
                        vkDestroyImageView(device, swapChainImageViews[i], alloc);
                }
                swapChainImageViews.clear();
                swapChainImages.clear();

                // TODO allocator callbacks
                vkDestroySwapchainKHR(device, swapChain, alloc);
        }

        void SwapChain::init()
        {
                createSwapChain();
                createImageViews();
                // createDepthResources();
        }

        void SwapChain::createSwapChain()
        {
                SwapChainSupportDetails swapChainSupport = device.getSwapChainSupport(device.physicalDevice);

                VkSurfaceFormatKHR surfaceFormat         = chooseSwapSurfaceFormat(swapChainSupport.formats);
                VkPresentModeKHR presentMode             = chooseSwapPresentMode(swapChainSupport.presentModes);
                VkExtent2D extent                        = chooseSwapExtent(swapChainSupport.capabilities);

                uint32_t imageCount                      = swapChainSupport.capabilities.minImageCount + 1;
                if (swapChainSupport.capabilities.maxImageCount > 0 &&
                    imageCount > swapChainSupport.capabilities.maxImageCount)
                {
                        imageCount = swapChainSupport.capabilities.maxImageCount;
                }

                VkSwapchainCreateInfoKHR createInfo = {};
                createInfo.sType                    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
                createInfo.surface                  = device.surface;

                createInfo.minImageCount            = imageCount;
                createInfo.imageFormat              = surfaceFormat.format;
                createInfo.imageColorSpace          = surfaceFormat.colorSpace;
                createInfo.imageExtent              = extent;
                createInfo.imageArrayLayers         = 1;
                createInfo.imageUsage               = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

                // TODO: Logging and needs fixing.
                uint32_t queueFamilyIndices[] = {device.queues.graphics.index, device.queues.present.index};

                if (device.queues.graphics.index != device.queues.present.index)
                {
                        createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
                        createInfo.queueFamilyIndexCount = 2;
                        createInfo.pQueueFamilyIndices   = queueFamilyIndices;
                }
                else
                {
                        createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
                        createInfo.queueFamilyIndexCount = 0;        // Optional
                        createInfo.pQueueFamilyIndices   = nullptr;  // Optional
                }

                createInfo.preTransform   = swapChainSupport.capabilities.currentTransform;
                createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

                createInfo.presentMode    = presentMode;
                createInfo.clipped        = VK_TRUE;

                createInfo.oldSwapchain   = oldSwapChain == nullptr ? VK_NULL_HANDLE : oldSwapChain->swapChain;

                if (vkCreateSwapchainKHR(device.logicalDevice, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
                {
                        IC_CORE_ERROR("Failed to create swapchain!");
                }

                // we only specified a minimum number of images in the swap chain, so the implementation is
                // allowed to create a swap chain with more. That's why we'll first query the final number of
                // images with vkGetSwapchainImagesKHR, then resize the container and finally call it again to
                // retrieve the handles.
                vkGetSwapchainImagesKHR(device.logicalDevice, swapChain, &imageCount, nullptr);
                swapChainImages.resize(imageCount);
                vkGetSwapchainImagesKHR(device.logicalDevice, swapChain, &imageCount, swapChainImages.data());

                swapChainImageFormat = surfaceFormat.format;
                swapChainExtent      = extent;
        }

        void SwapChain::createImageViews()
        {
                swapChainImageViews.resize(swapChainImages.size());

                for (size_t i = 0; i < swapChainImageViews.size(); i++)
                {
                        VkImageViewCreateInfo createInfo{};
                        createInfo.sType        = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                        createInfo.image        = swapChainImages[i];

                        createInfo.viewType     = VK_IMAGE_VIEW_TYPE_2D;  // image view in 1d 2d or 3d or cube maps
                        createInfo.format       = swapChainImageFormat;

                        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
                        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
                        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
                        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

                        createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                        createInfo.subresourceRange.baseMipLevel   = 0;  // edit mipmap levels hmmm????
                        createInfo.subresourceRange.levelCount     = 1;
                        createInfo.subresourceRange.baseArrayLayer = 0;
                        createInfo.subresourceRange.layerCount     = 1;

                        if (vkCreateImageView(device.logicalDevice, &createInfo, nullptr, &swapChainImageViews[i]) !=
                            VK_SUCCESS)
                        {
                                IC_ERROR("Failed to Create Image Views");
                        }
                }
        }

        // TODO compare this function with Depth stencil function and see if it one can go
        void SwapChain::createDepthResources()
        {
                VkFormat depthFormat       = findDepthFormat();
                swapChainDepthFormat       = depthFormat;
                VkExtent2D swapChainExtent = getSwapChainExtent();

                depthImages.resize(imageCount());
                depthImageMemorys.resize(imageCount());
                depthImageViews.resize(imageCount());

                for (int i = 0; i < depthImages.size(); i++)
                {
                        VkImageCreateInfo imageInfo{};
                        imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                        imageInfo.imageType     = VK_IMAGE_TYPE_2D;
                        imageInfo.extent.width  = swapChainExtent.width;
                        imageInfo.extent.height = swapChainExtent.height;
                        imageInfo.extent.depth  = 1;
                        imageInfo.mipLevels     = 1;
                        imageInfo.arrayLayers   = 1;
                        imageInfo.format        = depthFormat;
                        imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
                        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                        imageInfo.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
                        imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
                        imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
                        imageInfo.flags         = 0;

                        VkMemoryRequirements memRequirements;
                        vkGetImageMemoryRequirements(device.logicalDevice, depthImages[i], &memRequirements);

                        VkMemoryAllocateInfo allocInfo{};
                        allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                        allocInfo.allocationSize  = memRequirements.size;
                        allocInfo.memoryTypeIndex = device.getMemoryType(memRequirements.memoryTypeBits,
                                                                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

                        if (vkAllocateMemory(device.logicalDevice, &allocInfo, nullptr, &depthImageMemorys[i]) !=
                            VK_SUCCESS)
                        {
                                IC_CORE_ERROR("Failed to allocate image memory!");
                        }

                        if (vkBindImageMemory(device.logicalDevice, depthImages[i], depthImageMemorys[i], 0) !=
                            VK_SUCCESS)
                        {
                                IC_CORE_ERROR("Failed to bind image memory!");
                        }

                        VkImageViewCreateInfo viewInfo{};
                        viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                        viewInfo.image                           = depthImages[i];
                        viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
                        viewInfo.format                          = depthFormat;
                        viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
                        viewInfo.subresourceRange.baseMipLevel   = 0;
                        viewInfo.subresourceRange.levelCount     = 1;
                        viewInfo.subresourceRange.baseArrayLayer = 0;
                        viewInfo.subresourceRange.layerCount     = 1;

                        if (vkCreateImageView(device.logicalDevice, &viewInfo, nullptr, &depthImageViews[i]) !=
                            VK_SUCCESS)
                        {
                                IC_CORE_ERROR("Failed to create texture image view!");
                        }
                }
        }

        VkSurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
        {
                for (const auto& availableFormat : availableFormats)
                {
                        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                        {
                                return availableFormat;  // return the format required.
                        }
                }
                return availableFormats[0];
        }

        VkPresentModeKHR SwapChain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
        {
                for (const auto& availablePresentMode : availablePresentModes)
                {
                        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
                        {
                                return availablePresentMode;
                        }
                }

                return VK_PRESENT_MODE_FIFO_KHR;
        }

        VkExtent2D SwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
        {
                if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
                {
                        return capabilities.currentExtent;
                }
                else
                {
                        int width, height;
                        glfwGetFramebufferSize(
                                                   Application::Get().GetWindow()->GetNativeWindow(),
                                               &width,
                                               &height);

                        VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

                        actualExtent.width      = std::clamp(actualExtent.width,
                                                        capabilities.minImageExtent.width,
                                                        capabilities.maxImageExtent.width);
                        actualExtent.height     = std::clamp(actualExtent.height,
                                                         capabilities.minImageExtent.height,
                                                         capabilities.maxImageExtent.height);

                        return actualExtent;
                }
        }

}  // namespace ic
