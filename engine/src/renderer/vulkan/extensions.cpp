#include "extensions.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

bool ic::vk::extensions::checkInstanceExtensionSupport(const std::vector<const char*>& requiredExtensions)
{
        uint32_t extCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
        std::vector<VkExtensionProperties> available(extCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extCount, available.data());

        for (const char* extName : requiredExtensions)
        {
                bool found = false;
                for (const auto& ext : available)
                {
                        if (strcmp(ext.extensionName, extName) == 0)
                        {
                                found = true;
                                break;
                        }
                }
                if (!found)
                {
                        IC_CORE_ERROR("Missing instance extension: {0}", extName);
                        return false;
                }
        }
        return true;
}

bool ic::vk::extensions::checkValidationLayerSupport(const std::vector<const char*>& requiredLayers)
{
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> available(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, available.data());

        for (const char* layerName : requiredLayers)
        {
                bool found = false;
                for (const auto& layer : available)
                {
                        if (strcmp(layerName, layer.layerName) == 0)
                        {
                                found = true;
                                break;
                        }
                }
                if (!found)
                {
                        IC_ERROR("Missing Validation Layer: {0}", layerName);
                        return false;
                }
        }
        return true;
}

std::vector<const char*> ic::vk::extensions::getRequiredInstanceExtensions(bool enableValidation)
{
        uint32_t glfwExtCount = 0;
        const char** glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);

        std::vector<const char*> extensions(glfwExts, glfwExts + glfwExtCount);

        if (enableValidation)
        {
                extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
}
