#pragma once

#include "defines.h"

// Vulkan Test
void testVulkanExtensions() {
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::cout << extensionCount << " extensions supported\n";
}

// uint32_t physical_device::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
//         IC_CORE_INFO("Finding memory type with filter: 0x{:X}, properties: 0x{:X}", typeFilter, properties);
    
//         for (uint32_t i = 0; i < m_memoryProperties.memoryTypeCount; i++) {
//             bool typeMatches = (typeFilter & (1 << i)) != 0;
//             bool propertiesMatch = (m_memoryProperties.memoryTypes[i].propertyFlags & properties) == properties;
            
//             IC_CORE_INFO("Memory type {}: filter match = {}, properties match = {} (flags: 0x{:X})", 
//                 i, typeMatches, propertiesMatch, m_memoryProperties.memoryTypes[i].propertyFlags);
            
//             if (typeMatches && propertiesMatch) {
//                 IC_CORE_INFO("Selected memory type: {}", i);
//                 return i;
//             }
//         }
//         IC_CORE_CRITICAL("Failed to find suitable memory type!");
//         IC_CORE_CRITICAL("    Requested filter: 0x{:X}", typeFilter);
//         IC_CORE_CRITICAL("    Requested properties: 0x{:X}", properties);
//         IC_CORE_CRITICAL("    Available memory types: {}", m_memoryProperties.memoryTypeCount);
        
//         for (uint32_t i = 0; i < m_memoryProperties.memoryTypeCount; i++) {
//             IC_CORE_CRITICAL(
//                 "    Type {}: heap {}, flags 0x{:X}", 
//                 i,
//                 m_memoryProperties.memoryTypes[i].heapIndex,
//                 m_memoryProperties.memoryTypes[i].propertyFlags
//             );
//         }
        
//         return UINT32_MAX; // never reached
//     }

//     VkFormat physical_device::findSupportedFormat(
//         const std::vector<VkFormat>& candidates,
//         VkImageTiling tiling,
//         VkFormatFeatureFlags features
//     ) const {
//         IC_CORE_INFO("Finding supported format from {} candidates", candidates.size());
//         IC_CORE_INFO("  Tiling: {}, Required features: 0x{:X}", 
//             tiling == VK_IMAGE_TILING_LINEAR ? "LINEAR" : "OPTIMAL", features);
        
//         for (size_t i = 0; i < candidates.size(); ++i) {
//             VkFormat format = candidates[i];
//             VkFormatProperties props;
//             // vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);
            
//             IC_CORE_INFO("Format {} ({}): linear features = 0x{:X}, optimal features = 0x{:X}", 
//                 i, static_cast<int>(format), props.linearTilingFeatures, props.optimalTilingFeatures);
        
//             if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
//                 IC_CORE_INFO("Selected format {} (linear tiling)", static_cast<int>(format));
//                 return format;
//             } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
//                 IC_CORE_INFO("Selected format {} (optimal tiling)", static_cast<int>(format));
//                 return format;
//             }
//         }
        
//         IC_CORE_ERROR("Failed to find supported format!");
//         IC_CORE_ERROR("  Candidates tested: {}", candidates.size());
//         IC_CORE_ERROR("  Tiling mode: {}", tiling == VK_IMAGE_TILING_LINEAR ? "LINEAR" : "OPTIMAL");
//         IC_CORE_ERROR("  Required features: 0x{:X}", features);
//         return VK_FORMAT_UNDEFINED; // never reached
//     }