#include "texture.h"
#include "vkutils/imageutils.h"

#include <ktx.h>
#include <ktxvulkan.h>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

namespace vkLoad
{

        void Texture::updateDescriptor()
        {
                descriptor.sampler     = sampler;
                descriptor.imageView   = view;
                descriptor.imageLayout = imageLayout;
        }

        void Texture::destroy()
        {
                if (device)
                {
                        vkDestroyImageView(device->logicalDevice, view, nullptr);
                        vkDestroyImage(device->logicalDevice, image, nullptr);
                        vkFreeMemory(device->logicalDevice, deviceMemory, nullptr);
                        vkDestroySampler(device->logicalDevice, sampler, nullptr);
                }
        }

        void Texture::fromglTfImage(tinygltf::Image& gltfimage,
                                    std::string path,
                                    TextureSampler textureSampler,
                                    ic::vkdevice* device,
                                    VkQueue copyQueue)
        {
                this->device = device;
                bool isKTX   = false;

                // if gltf image points to an external ktx file.
                if (gltfimage.uri.find_last_of(".") != std::string::npos)
                {
                        if (gltfimage.uri.substr(gltfimage.uri.find_last_of(".") + 1) == "ktx")
                                isKTX = true;
                }

                // [TODO IMP] Set format from outside
                VkFormat format;

                if (!isKTX)
                {
                        // Texture loading with STB_image because simpler
                        unsigned char* buffer   = nullptr;
                        VkDeviceSize bufferSize = 0;
                        bool deleteBuffer       = false;

                        // If the image has only 3 components convert to 4
                        if (gltfimage.component == 3)
                        {
                                bufferSize          = gltfimage.width * gltfimage.height * 4;
                                buffer              = new unsigned char[bufferSize];
                                unsigned char* rgba = buffer;
                                unsigned char* rgb  = &gltfimage.image[0];

                                for (size_t i = 0; i < gltfimage.width * gltfimage.height; ++i)
                                {
                                        for (uint32_t j = 0; j < 3; ++j)
                                        {
                                                rgba[j] = rgb[j];
                                        }
                                        rgba += 4;
                                        rgb  += 3;
                                }
                                deleteBuffer = true;
                        }
                        else
                        {
                                buffer     = &gltfimage.image[0];
                                bufferSize = gltfimage.image.size();
                        }
                        assert(buffer);

                        format    = VK_FORMAT_R8G8B8A8_UNORM;

                        width     = gltfimage.width;
                        height    = gltfimage.height;
                        mipLevels = getMipLevels();

                        // Make sure bliting is exists on device (Blitting  is the process of sending rectangle data
                        // from image directly to the buffers)
                        VkFormatProperties formatProperties{};
                        vkGetPhysicalDeviceFormatProperties(device->physicalDevice, format, &formatProperties);
                        assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT);
                        assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT);

                        // Make a vulkan staging buffer for the image
                        VkBuffer stagingBuffer;
                        VkDeviceMemory stagingMemory;

                        VkBufferCreateInfo bufCreateInfo{};
                        bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                        bufCreateInfo.size  = bufferSize;
                        bufCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

                        IC_CORE_ASSERT(vkCreateBuffer(device->logicalDevice, &bufCreateInfo, nullptr, &stagingBuffer) ==
                                           VK_SUCCESS,
                                       "Couldn't Create Buffer for Image");

                        VkMemoryRequirements mem;
                        vkGetBufferMemoryRequirements(device->logicalDevice, stagingBuffer, &mem);
                        VkMemoryAllocateInfo allocInfo{};
                        allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                        allocInfo.allocationSize  = mem.size;
                        allocInfo.memoryTypeIndex = device->getMemoryType(mem.memoryTypeBits,
                                                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

                        IC_CORE_ASSERT(vkAllocateMemory(device->logicalDevice, &allocInfo, nullptr, &stagingMemory) ==
                                           VK_SUCCESS,
                                       "Failed to allocate Image Memory");
                        IC_CORE_ASSERT(vkBindBufferMemory(device->logicalDevice, stagingBuffer, stagingMemory, 0) ==
                                           VK_SUCCESS,
                                       "Couldn't Bind the Image Buffer");

                        // bind the device local buffer to cpu local to map to the image
                        uint8_t* data = nullptr;
                        IC_CORE_ASSERT(
                            vkMapMemory(device->logicalDevice, stagingMemory, 0, mem.size, 0, (void**)&data) ==
                                VK_SUCCESS,
                            "Failed to Map Memory");
                        memcpy(data, buffer, bufferSize);
                        vkUnmapMemory(device->logicalDevice, stagingMemory);

                        // Create optimal tiled target image
                        VkImageCreateInfo imageCreateInfo{};
                        imageCreateInfo.sType       = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                        imageCreateInfo.imageType   = VK_IMAGE_TYPE_2D;
                        imageCreateInfo.format      = VK_FORMAT_R8G8B8A8_UNORM;
                        imageCreateInfo.extent      = {.width = width, .height = height, .depth = 1};
                        imageCreateInfo.mipLevels   = mipLevels;
                        imageCreateInfo.arrayLayers = 1;
                        imageCreateInfo.samples     = VK_SAMPLE_COUNT_1_BIT;
                        imageCreateInfo.tiling      = VK_IMAGE_TILING_OPTIMAL;
                        imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                                VK_IMAGE_USAGE_SAMPLED_BIT;
                        imageCreateInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
                        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

                        IC_CORE_ASSERT(vkCreateImage(device->logicalDevice, &imageCreateInfo, nullptr, &image) ==
                                           VK_SUCCESS,
                                       "Failed to Create Image");
                        vkGetImageMemoryRequirements(device->logicalDevice,
                                                     image,
                                                     &mem);  // Reinitialize the requirements
                        allocInfo.allocationSize  = mem.size;
                        allocInfo.memoryTypeIndex = device->getMemoryType(mem.memoryTypeBits,
                                                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

                        IC_CORE_ASSERT(vkAllocateMemory(device->logicalDevice, &allocInfo, nullptr, &deviceMemory) ==
                                           VK_SUCCESS,
                                       "Failed to allocate Image Memory.");
                        IC_CORE_ASSERT(vkBindImageMemory(device->logicalDevice, image, deviceMemory, 0) == VK_SUCCESS,
                                       "Couldn't Bind the Image Memory.");

                        VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
                        ic::utils::transitionImageLayout(
                            copyCmd, image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

                        ic::utils::copyBufferToImage(copyCmd, stagingBuffer, image, width, height);

                        ic::utils::transitionImageLayout(copyCmd,
                                                         image,
                                                         format,
                                                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                         VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

                        device->flushCommandBuffer(copyCmd, copyQueue, true);
                        vkDestroyBuffer(device->logicalDevice, stagingBuffer, nullptr);
                        vkFreeMemory(device->logicalDevice, stagingMemory, nullptr);

                        // Generate the mip chain (glTF uses jpg and png, so we need to create this manually)
                        VkCommandBuffer blitCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

                        for (uint32_t i = 0; i < mipLevels; i++)
                        {
                                VkImageBlit imageBlit{};
                                imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                                imageBlit.srcSubresource.mipLevel   = i - 1;
                                imageBlit.srcSubresource.layerCount = 1;

                                imageBlit.srcOffsets[1].x           = int32_t(width >> (i - 1));
                                imageBlit.srcOffsets[1].y           = int32_t(height >> (i - 1));
                                imageBlit.srcOffsets[1].z           = 1;

                                imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                                imageBlit.dstSubresource.mipLevel   = i;
                                imageBlit.dstSubresource.layerCount = 1;

                                imageBlit.dstOffsets[1].x           = int32_t(width >> i);
                                imageBlit.dstOffsets[1].y           = int32_t(height >> i);
                                imageBlit.dstOffsets[1].z           = 1;

                                ic::utils::transitionImageLayout(blitCmd,
                                                                 image,
                                                                 format,
                                                                 VK_IMAGE_LAYOUT_UNDEFINED,
                                                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

                                vkCmdBlitImage(blitCmd,
                                               image,
                                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                               image,
                                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                               1,
                                               &imageBlit,
                                               VK_FILTER_LINEAR);

                                ic::utils::transitionImageLayout(blitCmd,
                                                                 image,
                                                                 format,
                                                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
                        }

                        // [TODO: Improve the transition layout function with more optimality and cases instead]
                        VkImageSubresourceRange subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                                                 .levelCount = mipLevels,
                                                                 .layerCount = 1};

                        VkImageMemoryBarrier imageMemoryBarrier{.sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                                                                .srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                                                                .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                                                                .oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                                                .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                                .image     = image,
                                                                .subresourceRange = subresourceRange};

                        imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                        vkCmdPipelineBarrier(blitCmd,
                                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                             0,
                                             0,
                                             nullptr,
                                             0,
                                             nullptr,
                                             1,
                                             &imageMemoryBarrier);
                        if (deleteBuffer)
                        {
                                delete[] buffer;
                        }

                        device->flushCommandBuffer(blitCmd, copyQueue, true);
                }
                else
                {
                        // get the ktx path from the gltf file
                        std::string filename = path + "/" + gltfimage.uri;

                        ktxTexture* texture;
                        ktxResult result = KTX_SUCCESS;

                        // [TODO]: Add this to utils
                        // [TODO IMP] Make a files system
                        std::ifstream f(filename.c_str());
                        if (f.fail())
                        {
                                IC_CORE_ERROR("Couldn't Open KTX file from the Referenced GLTF file");
                        }
                        result = ktxTexture_CreateFromNamedFile(filename.c_str(),
                                                                KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                                                &texture);

                        IC_CORE_ASSERT(result == KTX_SUCCESS, "KTX Texture creation Failed");

                        this->device                = device;
                        width                       = texture->baseWidth;
                        height                      = texture->baseHeight;
                        mipLevels                   = texture->numLevels;

                        ktx_uint8_t* ktxTextureData = ktxTexture_GetData(texture);
                        ktx_size_t ktxTextureSize   = ktxTexture_GetDataSize(texture);
                        format                      = ktxTexture_GetVkFormat(texture);

                        VkFormatProperties formatProperties{};
                        vkGetPhysicalDeviceFormatProperties(device->physicalDevice, format, &formatProperties);

                        VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

                        VkBuffer stagingBuffer;
                        VkDeviceMemory stagingMemory;

                        VkBufferCreateInfo bufCreateInfo{};
                        bufCreateInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                        bufCreateInfo.size        = ktxTextureSize;
                        bufCreateInfo.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                        bufCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                        IC_CORE_ASSERT(vkCreateBuffer(device->logicalDevice, &bufCreateInfo, nullptr, &stagingBuffer) ==
                                           VK_SUCCESS,
                                       "Buffer Creation Failed.");

                        VkMemoryRequirements memoryRequirements{};
                        vkGetBufferMemoryRequirements(device->logicalDevice, stagingBuffer, &memoryRequirements);
                        VkMemoryAllocateInfo allocInfo{};
                        allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                        allocInfo.allocationSize  = memoryRequirements.size;
                        allocInfo.memoryTypeIndex = device->getMemoryType(memoryRequirements.memoryTypeBits,
                                                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                        IC_CORE_ASSERT(vkAllocateMemory(device->logicalDevice, &allocInfo, nullptr, &stagingMemory) ==
                                           VK_SUCCESS,
                                       "Failed to allocate memory!");
                        IC_CORE_ASSERT(vkBindBufferMemory(device->logicalDevice, stagingBuffer, stagingMemory, 0) ==
                                           VK_SUCCESS,
                                       "Failed to Bind buffer memory");

                        uint8_t* data{nullptr};
                        IC_CORE_ASSERT(vkMapMemory(device->logicalDevice,
                                                   stagingMemory,
                                                   0,
                                                   memoryRequirements.size,
                                                   0,
                                                   (void**)&data) == VK_SUCCESS,
                                       "Failed to map memory!");
                        memcpy(data, ktxTextureData, ktxTextureSize);
                        vkUnmapMemory(device->logicalDevice, stagingMemory);

                        std::vector<VkBufferImageCopy> bufferCopyRegions;
                        for (uint32_t i = 0; i < mipLevels; i++)
                        {
                                ktx_size_t offset;
                                KTX_error_code result = ktxTexture_GetImageOffset(texture, i, 0, 0, &offset);
                                IC_CORE_ASSERT(result == KTX_SUCCESS, "Texture Image Offset Failed");
                                VkBufferImageCopy bufferCopyRegion{};
                                bufferCopyRegion.bufferOffset                    = offset;
                                bufferCopyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                                bufferCopyRegion.imageSubresource.mipLevel       = i;
                                bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
                                bufferCopyRegion.imageSubresource.layerCount     = 1;
                                bufferCopyRegion.imageExtent.width  = std::max(1u, texture->baseWidth >> i);
                                bufferCopyRegion.imageExtent.height = std::max(1u, texture->baseHeight >> i);
                                bufferCopyRegion.imageExtent.depth  = 1;

                                bufferCopyRegions.push_back(bufferCopyRegion);
                        }

                        VkImageCreateInfo imageCreateInfo{};
                        imageCreateInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                        imageCreateInfo.imageType     = VK_IMAGE_TYPE_2D;
                        imageCreateInfo.format        = format;
                        imageCreateInfo.extent        = {.width = width, .height = height, .depth = 1};
                        imageCreateInfo.mipLevels     = mipLevels;
                        imageCreateInfo.arrayLayers   = 1;
                        imageCreateInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
                        imageCreateInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
                        imageCreateInfo.usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                        imageCreateInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
                        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

                        IC_CORE_ASSERT(vkCreateImage(device->logicalDevice, &imageCreateInfo, nullptr, &image) ==
                                           VK_SUCCESS,
                                       "Failed to create Image");

                        vkGetImageMemoryRequirements(device->logicalDevice, image, &memoryRequirements);
                        allocInfo.allocationSize  = memoryRequirements.size;
                        allocInfo.memoryTypeIndex = device->getMemoryType(memoryRequirements.memoryTypeBits,
                                                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

                        IC_CORE_ASSERT(vkAllocateMemory(device->logicalDevice, &allocInfo, nullptr, &deviceMemory) ==
                                           VK_SUCCESS,
                                       "Failed to allocate Image memory!");
                        IC_CORE_ASSERT(vkBindImageMemory(device->logicalDevice, image, deviceMemory, 0) == VK_SUCCESS,
                                       "Failed to Bind Image memory");

                        ic::utils::transitionImageLayout(
                            copyCmd, image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
                        vkCmdCopyBufferToImage(copyCmd,
                                               stagingBuffer,
                                               image,
                                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                               static_cast<uint32_t>(bufferCopyRegions.size()),
                                               bufferCopyRegions.data());
                        ic::utils::transitionImageLayout(copyCmd,
                                                         image,
                                                         format,
                                                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                        device->flushCommandBuffer(copyCmd, copyQueue);
                        this->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                        vkDestroyBuffer(device->logicalDevice, stagingBuffer, nullptr);
                        vkFreeMemory(device->logicalDevice, stagingMemory, nullptr);

                        ktxTexture_Destroy(texture);
                }

                // [TODO IMP Make sure to edit this through an API]
                VkSamplerCreateInfo samplerInfo{};
                samplerInfo.sType            = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                samplerInfo.magFilter        = VK_FILTER_LINEAR;
                samplerInfo.minFilter        = VK_FILTER_LINEAR;
                samplerInfo.mipmapMode       = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                samplerInfo.addressModeU     = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
                samplerInfo.addressModeV     = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
                samplerInfo.addressModeW     = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
                samplerInfo.anisotropyEnable = VK_TRUE;
                samplerInfo.maxAnisotropy    = 8.0f;
                samplerInfo.compareOp        = VK_COMPARE_OP_NEVER;
                samplerInfo.maxLod           = (float)mipLevels;
                samplerInfo.borderColor      = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

                IC_CORE_ASSERT(vkCreateSampler(device->logicalDevice, &samplerInfo, nullptr, &sampler) == VK_SUCCESS,
                               "Failed to create Sampler");

                VkImageViewCreateInfo viewInfo{};
                viewInfo.sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                viewInfo.image                       = image;
                viewInfo.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
                viewInfo.format                      = format;
                viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                viewInfo.subresourceRange.levelCount = mipLevels;
                viewInfo.subresourceRange.layerCount = 1;

                if (vkCreateImageView(device->logicalDevice, &viewInfo, nullptr, &view) != VK_SUCCESS)
                {
                        IC_CORE_ERROR("Failed to create Image view");
                }

                descriptor.sampler     = sampler;
                descriptor.imageView   = view;
                descriptor.imageLayout = imageLayout;
        }

}  // namespace vkLoad
