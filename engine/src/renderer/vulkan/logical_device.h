#pragma once
#include "defines.h"
#include "physical_device.h"
#include "queue_manager.h"

namespace ic {
    
    class logical_device {
    private:
        VkDevice m_device = VK_NULL_HANDLE;
        physical_device m_physicalDevice;
        queue_manager* m_queueManager; // TODO: dont use unique pointers baby
        
    public:
        logical_device() = default;
        ~logical_device();
        
        // Move semantics (RAII)
        logical_device(const logical_device&) = delete;
        logical_device& operator=(const logical_device&) = delete;
        logical_device(logical_device&& other) noexcept;
        logical_device& operator=(logical_device&& other) noexcept;
        
        bool create(const physical_device& physicalDevice, VkSurfaceKHR surface);
        // // Enable features you need
        // requirements.requiredFeatures.samplerAnisotropy = VK_TRUE;
        // requirements.requiredFeatures.fillModeNonSolid = VK_TRUE;
        // requirements.requiredFeatures.wideLines = VK_TRUE;
        
        // // For high-performance applications
        // requirements.requiresDedicatedGPU = true;
        // requirements.minVulkanVersion = VK_API_VERSION_1_2;

        void destroy();
        
        VkDevice get() const { return m_device; }
        operator VkDevice() const { return m_device; }
        
        const physical_device& getPhysicalDevice() const { return m_physicalDevice; }
        // queue_manager* getQueueManager() const { return m_queueManager->get(); }
        
        void waitIdle() const;
        
    private:

        bool createLogicalDevice(VkSurfaceKHR surface);
        std::vector<const char*> getRequiredExtensionPtrs() const;

        void moveFrom(logical_device&& other) noexcept;
        void reset() noexcept;
    };
    
} // namespace ic