#ifndef
#define

#include "defines.h"

namespace ic
{
namespace utils
{
inline bool hasStencilComponent(VkFormat format)
{
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

inline void copyBufferToImage(
    VkCommandBuffer copyCmd, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t baseMip)
{

        VkBufferImageCopy region{};
        region.bufferOffset                    = 0;
        region.bufferRowLength                 = 0;
        region.bufferImageHeight               = 0;

        region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel       = baseMip;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount     = 1;

        region.imageOffset                     = {0, 0, 0};
        region.imageExtent                     = {width, height, 1};

        vkCmdCopyBufferToImage(copyCmd, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

inline VkImageSubresourceRange mipRange(uint32_t baseMip, uint32_t levelCount = 1)
{
        VkImageSubresourceRange r{};
        r.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        r.baseMipLevel   = baseMip;
        r.levelCount     = levelCount;
        r.baseArrayLayer = 0;
        r.layerCount     = 1;
        return r;
}

// Transitions for the common paths used in upload + mips
inline void transitionLayout(
    VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange range)
{
        VkImageMemoryBarrier b{};
        b.sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.oldLayout                   = oldLayout;
        b.newLayout                   = newLayout;
        b.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
        b.image                       = image;
        b.subresourceRange            = range;

        VkPipelineStageFlags srcStage = 0, dstStage = 0;

        // match cases:
        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
                b.srcAccessMask = 0;
                b.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                srcStage        = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                dstStage        = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
        {
                b.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                b.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                srcStage        = VK_PIPELINE_STAGE_TRANSFER_BIT;
                dstStage        = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL &&
                 newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
                b.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                srcStage        = VK_PIPELINE_STAGE_TRANSFER_BIT;
                dstStage        = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;  // or ALL_COMMANDS if shared
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
                 newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
                b.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                srcStage        = VK_PIPELINE_STAGE_TRANSFER_BIT;
                dstStage        = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else
        {
                IC_CORE_ERROR("Unsupported layout transition");
        }

        IC_CORE_TRACE("Image layout transition: {} -> {}", (uint32_t)oldLayout, (uint32_t)newLayout);
        vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &b);
}

}  // namespace utils

}  // namespace ic