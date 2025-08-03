#pragma once
#include "defines.h"

namespace ic
{ 
    class vulkan_context {
    public:
        bool init();
        void cleanUp();
        
        VkInstance getInstance() const { return m_instance; }

    private:
        bool createInstance();

        VkInstance m_instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
        
        // debug utils
#ifdef _DEBUG
        const bool m_enableValidationLayers = true;
#else
        const bool m_enableValidationLayers = false;
#endif
        const std::vector<const char*> m_validationLayers = {
            "VK_LAYER_KHRONOS_validation"
        };
    };
} // namespace ic
