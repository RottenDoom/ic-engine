#include "buffer.h"

namespace ic
{
        buffer::buffer(VkDevice& logicalDevice) : device(logicalDevice) {}

        VkResult buffer::map(VkDeviceSize size, VkDeviceSize offset)
        {
                return vkMapMemory(device, memory, offset, size, 0, &mapped);
        }

        void buffer::unmap()
        {
                if (mapped)
                {
                        vkUnmapMemory(device, memory);
                        mapped = nullptr;
                }
        }

        VkResult buffer::bind(VkDeviceSize offset)
        {
                return vkBindBufferMemory(device, handle, memory, offset);
        }

        void buffer::setupDescriptor(VkDeviceSize size, VkDeviceSize offset)
        {
                descriptor.offset = offset;
                descriptor.buffer = handle;
                descriptor.range  = size;
        }

        void buffer::copyTo(void* data, VkDeviceSize size)
        {
                IC_ASSERT(mapped == nullptr, "Buffer Mapping does not exist.");
                memcpy(mapped, data, size);
        }

        VkResult buffer::flush(VkDeviceSize size, VkDeviceSize offset)
        {
                VkMappedMemoryRange mappedRange{.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
                                                .memory = memory,
                                                .offset = offset,
                                                .size   = size};
                return vkFlushMappedMemoryRanges(device, 1, &mappedRange);
        }

        VkResult buffer::invalidate(VkDeviceSize size, VkDeviceSize offset)
        {
                VkMappedMemoryRange mappedRange{.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
                                                .memory = memory,
                                                .offset = offset,
                                                .size   = size};
                return vkInvalidateMappedMemoryRanges(device, 1, &mappedRange);
        }

        void buffer::destroy()
        {
                if (handle)
                {
                        vkDestroyBuffer(device, handle, nullptr);
                        handle = VK_NULL_HANDLE;
                }
                if (memory)
                {
                        vkFreeMemory(device, memory, nullptr);
                        memory = VK_NULL_HANDLE;
                }
        }

}  // namespace ic
