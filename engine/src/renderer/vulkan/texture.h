#ifndef TEXTURE_H
#define TEXTURE_H

#include "defines.h"
#include "device.h"

#include <tiny_gltf.h>

namespace vkLoad
{
struct TextureSampler
{
        VkFilter magFilter;
        VkFilter minFilter;
        VkSamplerAddressMode addressModeU;
        VkSamplerAddressMode addressModeV;
        VkSamplerAddressMode addressModeW;
};

/** @brief A texture is just an image for color coordinates of an object. It contains and image mipmaps and a
 * sampler. A sampler is anything that samples and image in different ways. A mip map is deviding an object by 2
 * every next image.
 */
struct Texture
{
        ic::vkdevice *device;
        VkImage image;
        VkImageLayout imageLayout;
        VkDeviceMemory deviceMemory;
        VkImageView view;
        uint32_t width, height;
        uint32_t mipLevels;
        uint32_t layerCount;
        VkDescriptorImageInfo descriptor;
        VkSampler sampler;
        void updateDescriptor();
        uint32_t getMipLevels() { return static_cast<uint32_t>(floor(log2(std::max(width, height))) + 1.0); }
        void destroy();
        void fromglTfImage(tinygltf::Image &gltfimage,
                           std::string path,
                           TextureSampler textureSampler,
                           ic::vkdevice *device,
                           VkQueue copyQueue);
};
}  // namespace vkLoad

#endif
