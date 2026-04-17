#ifndef SYNC_OBJS_H
#define SYNC_OBJS_H

#include "defines.h"

#include <vulkan/vulkan.h>

namespace ic
{
struct fences
{
        std::vector<VkFence> inFlightFences;
        uint32_t framesInFlight;

        void create(const VkDevice &device, const uint32_t &maxFramesInFlight);
        void destroy(const VkDevice &device);
};

struct semaphores
{
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        uint32_t framesInflight;

        void create(const VkDevice &device, const uint32_t &maxFramesInFlight);
        void destroy(const VkDevice &device);
};
}  // namespace ic

#endif
