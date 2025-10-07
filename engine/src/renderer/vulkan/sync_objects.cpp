#include "sync_objects.h"

namespace ic
{
        void fences::create(const VkDevice& device, const uint32_t& maxFramesInFlight)
        {
                framesInFlight = maxFramesInFlight;
                inFlightFences.resize(framesInFlight);

                VkFenceCreateInfo CI{};
                CI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
                CI.flags = VK_FENCE_CREATE_SIGNALED_BIT;

                for (size_t i = 0; i < framesInFlight; i++)
                {
                        if (vkCreateFence(device, &CI, nullptr, &inFlightFences[i]) != VK_SUCCESS)
                        {
                                IC_CORE_ERROR("Failed to create fences(sync object)!");
                        }
                }
        }

        void fences::destroy(const VkDevice& device)
        {
                for (size_t i = 0; i < framesInFlight; i++)
                {
                        vkDestroyFence(device, inFlightFences[i], nullptr);
                }
                inFlightFences.clear();
        }

        void semaphores::create(const VkDevice& device, const uint32_t& maxFramesInFlight)
        {
                framesInflight = maxFramesInFlight;
                imageAvailableSemaphores.resize(framesInflight);
                renderFinishedSemaphores.resize(framesInflight);

                VkSemaphoreCreateInfo semaphoreInfo{};
                semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

                for (size_t i = 0; i < framesInflight; i++)
                {
                        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) !=
                                VK_SUCCESS ||
                            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) !=
                                VK_SUCCESS)
                        {

                                IC_CORE_ERROR("Failed to create semaphores for the frame (sync object)!");
                        }
                }
        }

        void semaphores::destroy(const VkDevice& device)
        {
                for (size_t i = 0; i < framesInflight; i++)
                {
                        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
                        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
                }
                imageAvailableSemaphores.clear();
                renderFinishedSemaphores.clear();
        }
} // namespace ic
