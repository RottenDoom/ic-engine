#pragma once
#include "defines.h"
#include "logical_device.h"
#include "surface.h"
#include "context.h"

namespace ic {
    
    struct swap_chain_support_details {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
        
        bool isAdequate() const {
            return !formats.empty() && !presentModes.empty();
        }
    };
    
    // RAII wrapper for swap chain with builder pattern for configuration
    class swapchain {
    private:
        VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
        std::vector<VkImage> m_images;
        std::vector<VkImageView> m_imageViews;
        VkPresentModeKHR m_presentMode;
        VkSurfaceFormatKHR m_imageFormat;
        VkFormat m_format;
        VkExtent2D m_extent;
        
        const logical_device* m_device = nullptr;
        const vulkan_surface* m_surface = nullptr;
        
    public:
        swapchain() = default;
        ~swapchain();
        
        // Move semantics (RAII)
        swapchain(const swapchain&) = delete;
        swapchain& operator=(const swapchain&) = delete;
        swapchain(swapchain&& other) noexcept;
        swapchain& operator=(swapchain&& other) noexcept;
        
        bool create(const logical_device& device, const vulkan_surface& surface, VkSwapchainKHR oldSwapChain = VK_NULL_HANDLE);
        void destroy();
        
        void recreate();
        
        VkSwapchainKHR get() const { return m_swapChain; }
        const std::vector<VkImage>& getImages() const { return m_images; }
        const std::vector<VkImageView>& getImageViews() const { return m_imageViews; }
        VkFormat getImageFormat() const { return m_format; }
        VkExtent2D getExtent() const { return m_extent; }
        
        uint32_t getImageCount() const { return static_cast<uint32_t>(m_images.size()); }
        
        // Static helper
        static swap_chain_support_details querySupport(VkPhysicalDevice device, VkSurfaceKHR surface);
        
    private:
        bool createImageViews();
        VkDevice getDevice() { return m_device->get(); }
        
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const;
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;
        
        void cleanUp();
    };
    
} // namespace ic