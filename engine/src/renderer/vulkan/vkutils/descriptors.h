#ifndef
#define

#include "defines.h"

namespace ic
{
namespace descriptor
{
inline VkDescriptorSetLayoutBinding createDescriptorSetLayoutBinding(uint32_t binding,
                                                                     VkDescriptorType type,
                                                                     VkShaderStageFlagBits stage)
{
        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding            = binding;
        layoutBinding.descriptorType     = type;
        layoutBinding.descriptorCount    = 1;
        layoutBinding.stageFlags         = stage;
        layoutBinding.pImmutableSamplers = nullptr;

        return layoutBinding;
}

// [TODO]: Improve this function as this just writes based on checks use cases instead here
inline VkWriteDescriptorSet writeDescriptorSet(
    VkDescriptorSet set, uint32_t binding, uint32_t arrayElement, VkDescriptorType type, const void *info)
{
        VkWriteDescriptorSet write{};
        write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet          = set;
        write.dstBinding      = binding;
        write.dstArrayElement = arrayElement;
        write.descriptorType  = type;
        write.descriptorCount = 1;

        if (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
        {
                write.pBufferInfo = (const VkDescriptorBufferInfo *)info;
        }
        else if (type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
        {
                write.pImageInfo = (const VkDescriptorImageInfo *)info;
        }
        else if (type == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER)
        {
                // DO something
        }

        return write;
}
}  // namespace descriptor

}  // namespace ic
