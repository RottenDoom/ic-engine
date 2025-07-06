#pragma once

#include "defines.h"
#include <GLFW/glfw3.h>

// Vulkan Test
void testVulkanExtensions() {
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::cout << extensionCount << " extensions supported\n";
}