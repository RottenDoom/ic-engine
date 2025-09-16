#pragma once
#include "defines.h"

namespace ic
{
        struct queue_family_indices
        {
                std::optional<uint32_t> graphicsFamily;
                std::optional<uint32_t> presentFamily;
                std::optional<uint32_t> computeFamily;
                std::optional<uint32_t> transferFamily;

                bool isComplete() const
                {
                        return graphicsFamily.has_value() && presentFamily.has_value();
                }

                std::set<uint32_t> getUniqueQueueFamilies() const
                {
                        std::set<uint32_t> uniqueFamilies;
                        if (graphicsFamily.has_value())
                                uniqueFamilies.insert(graphicsFamily.value());
                        if (presentFamily.has_value())
                                uniqueFamilies.insert(presentFamily.value());
                        if (computeFamily.has_value())
                                uniqueFamilies.insert(computeFamily.value());
                        if (transferFamily.has_value())
                                uniqueFamilies.insert(transferFamily.value());
                        return uniqueFamilies;
                }
        };

        // RAII queue manager with move-only semantics
        class queue_manager
        {
        private:
                VkDevice m_device = VK_NULL_HANDLE;
                queue_family_indices m_indices;
                VkQueue m_graphicsQueue = VK_NULL_HANDLE;
                VkQueue m_presentQueue  = VK_NULL_HANDLE;
                VkQueue m_computeQueue  = VK_NULL_HANDLE;
                VkQueue m_transferQueue = VK_NULL_HANDLE;

                void cleanup();

        public:
                queue_manager() = default;
                ~queue_manager();

                // Move-only semantics
                queue_manager(const queue_manager&)            = delete;
                queue_manager& operator=(const queue_manager&) = delete;
                queue_manager(queue_manager&& other) noexcept;
                queue_manager& operator=(queue_manager&& other) noexcept;

                // Initialization
                bool initialize(VkDevice device, const queue_family_indices& indices);
                bool isInitialized() const;

                // Queue access
                VkQueue getGraphicsQueue() const
                {
                        return m_graphicsQueue;
                }
                VkQueue getPresentQueue() const
                {
                        return m_presentQueue;
                }
                VkQueue getComputeQueue() const
                {
                        return m_computeQueue;
                }
                VkQueue getTransferQueue() const
                {
                        return m_transferQueue;
                }
                const queue_family_indices& getIndices() const
                {
                        return m_indices;
                }

                // Synchronization utilities
                void waitIdle() const;
                void waitGraphicsQueue() const;
                void waitPresentQueue() const;
                void waitComputeQueue() const;
                void waitTransferQueue() const;

                // Static helper for finding queue families
                static queue_family_indices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
        };
} // namespace ic