#include "swapchain.h"
#include "core/application.h"
#include "queue_manager.h"

namespace ic
{
        swapchain::swapchain(vkdevice& device, VkSurfaceKHR surface) : m_device(device), m_surface(surface) {}

        swapchain::~swapchain() {}

        // TODO: const correctness
        bool swapchain::create(VkSwapchainKHR oldSwapChain)
        {
                swap_chain_support_details swapChainSupport = querySupport(m_device.physicalDevice, m_surface);

                m_imageFormat                               = chooseSwapSurfaceFormat(swapChainSupport.formats);
                m_extent                                    = chooseSwapExtent(swapChainSupport.capabilities);
                m_presentMode                               = chooseSwapPresentMode(swapChainSupport.presentModes);

                uint32_t imageCount                         = swapChainSupport.capabilities.minImageCount + 1;

                if (swapChainSupport.capabilities.maxImageCount > 0 &&
                    imageCount > swapChainSupport.capabilities.maxImageCount)
                {
                        imageCount = swapChainSupport.capabilities.maxImageCount;
                }

                VkSwapchainCreateInfoKHR createInfo{};
                createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
                createInfo.surface          = m_surface;
                createInfo.minImageCount    = imageCount;
                createInfo.imageFormat      = m_imageFormat.format;
                createInfo.imageColorSpace  = m_imageFormat.colorSpace;
                createInfo.imageExtent      = m_extent;
                createInfo.imageArrayLayers = 1;
                createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

                // TODO: fix
                queue_family_indices indices  = queue_manager::findQueueFamilies(m_device.physicalDevice, m_surface);
                uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

                if (indices.graphicsFamily != indices.presentFamily)
                {
                        createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
                        createInfo.queueFamilyIndexCount = 2;
                        createInfo.pQueueFamilyIndices   = queueFamilyIndices;
                }
                else
                {
                        createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
                        createInfo.queueFamilyIndexCount = 0;
                        createInfo.pQueueFamilyIndices   = nullptr;
                }

                createInfo.preTransform   = swapChainSupport.capabilities.currentTransform;  // transform the image
                createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;  // alpha channel blending with other
                                                                                // windows.

                createInfo.presentMode  = m_presentMode;
                createInfo.clipped      = VK_TRUE;
                createInfo.oldSwapchain = oldSwapChain;

                if (vkCreateSwapchainKHR(m_device.logicalDevice, &createInfo, nullptr, &swapchainHandle) != VK_SUCCESS)
                {
                        IC_CORE_ERROR("Failed to create swapchain!");
                        return false;
                }

                vkGetSwapchainImagesKHR(m_device.logicalDevice, swapchainHandle, &imageCount, nullptr);
                m_images.resize(imageCount);
                vkGetSwapchainImagesKHR(m_device.logicalDevice, swapchainHandle, &imageCount, m_images.data());

                m_format = m_imageFormat.format;

                if (!createImageViews())
                {
                        IC_CORE_ERROR("Failed to create Image Views!");
                        return false;
                }

                return true;
        }

        void swapchain::destroy()
        {
                for (auto imageView : m_imageViews)
                {
                        vkDestroyImageView(m_device.logicalDevice, imageView, nullptr);
                }
                m_imageViews.clear();

                vkDestroySwapchainKHR(m_device.logicalDevice, swapchainHandle, nullptr);

                // TODO: test this shit
        }

        void swapchain::recreate()
        {
                vkDeviceWaitIdle(m_device.logicalDevice);

                // Save the old swap chain handle
                VkSwapchainKHR oldSwapChain = swapchainHandle;

                // Destroy old image views first (but not the swapchain yet)
                for (auto view : m_imageViews)
                {
                        if (view != VK_NULL_HANDLE)
                        {
                                vkDestroyImageView(m_device.logicalDevice, view, nullptr);
                        }
                }
                m_imageViews.clear();

                // Call create again with the old swapchain to reuse compatible resources
                if (!create(oldSwapChain))
                        ;
                {
                        IC_CORE_ERROR("failed to recreate swap chain.");
                }

                // Destroy the old swapchain AFTER creating the new one
                if (oldSwapChain != VK_NULL_HANDLE)
                {
                        vkDestroySwapchainKHR(m_device.logicalDevice, oldSwapChain, nullptr);
                }

                // Create image views for the new swap chain images
                createImageViews();
        }

        swap_chain_support_details swapchain::querySupport(VkPhysicalDevice device, VkSurfaceKHR surface)
        {
                swap_chain_support_details details;
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

        bool swapchain::createImageViews()
        {
                m_imageViews.resize(m_images.size());

                for (size_t i = 0; i < m_imageViews.size(); i++)
                {
                        VkImageViewCreateInfo createInfo{};
                        createInfo.sType        = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                        createInfo.image        = m_images[i];

                        createInfo.viewType     = VK_IMAGE_VIEW_TYPE_2D;  // image view in 1d 2d or 3d or cube maps
                        createInfo.format       = m_format;

                        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
                        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
                        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
                        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

                        createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                        createInfo.subresourceRange.baseMipLevel   = 0;  // edit mipmap levels hmmm????
                        createInfo.subresourceRange.levelCount     = 1;
                        createInfo.subresourceRange.baseArrayLayer = 0;
                        createInfo.subresourceRange.layerCount     = 1;

                        if (vkCreateImageView(m_device.logicalDevice, &createInfo, nullptr, &m_imageViews[i]) !=
                            VK_SUCCESS)
                        {
                                IC_ERROR("Failed to Create Image Views");
                                return false;
                        }
                }
                return true;
        }

        VkSurfaceFormatKHR
        swapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const
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

        VkPresentModeKHR
        swapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const
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

        VkExtent2D swapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const
        {
                if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
                {
                        return capabilities.currentExtent;
                }
                else
                {
                        int width, height;
                        glfwGetFramebufferSize(application::getWindow(), &width, &height);

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

        void swapchain::cleanUp() {}

}  // namespace ic
