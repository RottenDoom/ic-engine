#pragma once
#include "defines.h"

namespace ic
{
        class descriptor_pool
        {
        public:
                descriptor_pool() = default;
                ~descriptor_pool();

                descriptor_pool(const descriptor_pool&)            = delete;
                descriptor_pool& operator=(const descriptor_pool&) = delete;
                descriptor_pool(descriptor_pool&& other) noexcept;
                descriptor_pool& operator=(descriptor_pool&& other) noexcept;

                bool create(VkDevice device, const std::vector<VkDescriptorPoolSize>& poolSize, uint32_t maxSets,
                            VkDescriptorPoolCreateFlags flags = 0);
                void destroy() noexcept;
                bool allocateDescriptorSets(const std::vector<VkDescriptorSetLayout>& layouts,
                                            std::vector<VkDescriptorSet>& outSets);
                void freeDescriptorSets(const std::vector<VkDescriptorSet>& sets) noexcept;

                VkDescriptorPool get() const { return m_pool; }
                operator VkDescriptorPool() const { return m_pool; }

        private:
                VkDevice m_device = VK_NULL_HANDLE;
                VkDescriptorPool m_pool;

                void moveFrom(descriptor_pool&& other) noexcept;
                void reset() noexcept;
        };

        class pipeline_layout
        {
        public:
                pipeline_layout() = default;
                ~pipeline_layout();

                pipeline_layout(const pipeline_layout&)            = delete;
                pipeline_layout& operator=(const pipeline_layout&) = delete;
                pipeline_layout(pipeline_layout&& other) noexcept;
                pipeline_layout& operator=(pipeline_layout&& other) noexcept;

                bool create(VkDevice device,
                            const std::vector<std::vector<VkDescriptorSetLayoutBinding>>& bindingsPerSet,
                            const std::vector<VkPushConstantRange>& pushConstants = {},
                            uint32_t maxPersistentDescriptorSets                  = 0);

                void destroy() noexcept;

                VkPipelineLayout get() const { return m_pipelineLayout; }
                const std::vector<VkDescriptorSetLayout>& getDescriptorSetLayouts() const { return m_setLayouts; }

                bool allocateDescriptorSets(std::vector<VkDescriptorSet>& outSets);
                descriptor_pool* getDescriptorPool() { return m_descriptorPool.get(); }

        private:
                VkDevice m_device = VK_NULL_HANDLE;

                std::vector<VkDescriptorSetLayout> m_setLayouts;
                VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

                std::unique_ptr<descriptor_pool> m_descriptorPool;

                static bool createDescriptorSetLayout(VkDevice device,
                                                      const std::vector<VkDescriptorSetLayoutBinding>& bindings,
                                                      VkDescriptorSetLayout& outLayout);

                void moveFrom(pipeline_layout&& other) noexcept;
                void reset() noexcept;
        };
} // namespace ic