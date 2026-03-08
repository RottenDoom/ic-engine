#ifndef SWAPCHAIN_H
#define SWAPCHAIN_H

#include "defines.h"
#include "surface.h"
#include "device.h"
namespace ic
{

class SwapChain
{
public:
        SwapChain(ic::vkdevice &deviceRef, VkExtent2D extent);
        SwapChain(ic::vkdevice &deviceRef, VkExtent2D extent, SwapChain *oldSwapChain);
        ~SwapChain()                            = default;

        SwapChain(const SwapChain &)            = delete;
        SwapChain &operator=(const SwapChain &) = delete;

        VkSwapchainKHR &getSwapChain() { return swapChain; }
        VkImageView getImageView(int index) { return swapChainImageViews[index]; }
        size_t imageCount() { return swapChainImages.size(); }
        VkFormat getSwapChainImageFormat() { return swapChainImageFormat; }
        VkExtent2D getSwapChainExtent() { return swapChainExtent; }
        uint32_t width() { return swapChainExtent.width; }
        uint32_t height() { return swapChainExtent.height; }

        float extentAspectRatio()
        {
                return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);
        }
        VkFormat findDepthFormat();

        void destroy(VkDevice &device, const VkAllocationCallbacks *alloc = nullptr);

private:
        void init();
        void createSwapChain();
        void createImageViews();
        void createDepthResources();

        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

private:
        VkFormat swapChainImageFormat;
        VkFormat swapChainDepthFormat;
        VkExtent2D swapChainExtent;

        std::vector<VkImage> depthImages;
        std::vector<VkDeviceMemory> depthImageMemorys;
        std::vector<VkImageView> depthImageViews;
        std::vector<VkImage> swapChainImages;
        std::vector<VkImageView> swapChainImageViews;

        vkdevice &device;
        VkExtent2D windowExtent;

        VkSwapchainKHR swapChain;
        std::shared_ptr<SwapChain> oldSwapChain;
};
}  // namespace ic

#endif
