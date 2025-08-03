#include "context.h"
#include "extensions.h"
#include "debug.h"

namespace ic
{
    bool vulkan_context::init() {
        if (!createInstance()) {
            IC_CRITICAL("Failed to create Vulkan Context!");
            return false;
        }
        
        IC_CORE_INFO("Initializad Vulkan Context!");
        return true;
    }

    void vulkan_context::cleanUp() {
        if (m_enableValidationLayers && m_debugMessenger != VK_NULL_HANDLE) {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");

            if (func) {
                func(m_instance, m_debugMessenger, nullptr);
            }
        }

        if (m_instance != VK_NULL_HANDLE) {
            vkDestroyInstance(m_instance, nullptr);
        }

        IC_CORE_INFO("Destroyed Vulkan Context!");
    }

    bool vulkan_context::createInstance() {
        
        // vulkan app info
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Vulkan IC Engine";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "IC Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;
        
        // instance create info
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        
        // setup extensions
        auto extensions = vk::extensions::getRequiredInstanceExtensions(m_enableValidationLayers);
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();
        
        // validatoin layers setup
        if (m_enableValidationLayers && !vk::extensions::checkValidationLayerSupport(m_validationLayers)) {
            IC_CORE_ERROR("Validation layers requested but not available.");
            return false;
        }

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (m_enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
            createInfo.ppEnabledLayerNames = m_validationLayers.data();

            vk::debug::setupDebugMassenger(m_enableValidationLayers, debugCreateInfo);
            createInfo.pNext = &debugCreateInfo;
        } else {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }
        VkResult baby = vkCreateInstance(&createInfo, nullptr, &m_instance);
        if (baby != VK_SUCCESS) {
            IC_CORE_ERROR("Failed to Create Vulkan Instance");
            return false;
        }

        return true;

    }
} // namespace ic
