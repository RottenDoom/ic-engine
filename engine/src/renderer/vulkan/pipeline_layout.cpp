#include "pipeline_layout.h"

namespace ic
{
        descriptor_pool::~descriptor_pool() {}

        descriptor_pool::descriptor_pool(descriptor_pool&& other) noexcept
        {
                moveFrom(std::move(other));
        }

        descriptor_pool& descriptor_pool::operator=(descriptor_pool&& other) noexcept
        {
                if (this != &other)
                {
                        destroy();
                        moveFrom(std::move(other));
                }
                return *this;
        }

        bool descriptor_pool::create(VkDevice device, const std::vector<VkDescriptorPoolSize>& poolSizes,
                                     uint32_t maxSets, VkDescriptorPoolCreateFlags flags)
        {
                if (device == VK_NULL_HANDLE || poolSizes.empty() || maxSets == 0)
                        return false;
                m_device = device;

                VkDescriptorPoolCreateInfo poolInfo{};
                poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
                poolInfo.pPoolSizes    = poolSizes.data();
                poolInfo.maxSets       = maxSets;
                poolInfo.flags         = flags; // e.g. VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT

                if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_pool) != VK_SUCCESS)
                        return false;

                return true;
        }

        void descriptor_pool::destroy() noexcept
        {
                if (m_pool != VK_NULL_HANDLE && m_device != VK_NULL_HANDLE)
                {
                        vkDestroyDescriptorPool(m_device, m_pool, nullptr);
                }
                reset();
        }

        bool descriptor_pool::allocateDescriptorSets(const std::vector<VkDescriptorSetLayout>& layouts,
                                                     std::vector<VkDescriptorSet>& outSets)
        {
                if (m_pool == VK_NULL_HANDLE || layouts.empty())
                        return false;

                std::vector<VkDescriptorSetLayout> layoutCopies = layouts;
                outSets.resize(layouts.size());
                VkDescriptorSetAllocateInfo allocInfo{};
                allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                allocInfo.descriptorPool     = m_pool;
                allocInfo.descriptorSetCount = static_cast<uint32_t>(layoutCopies.size());
                allocInfo.pSetLayouts        = layoutCopies.data();

                VkResult result              = vkAllocateDescriptorSets(m_device, &allocInfo, outSets.data());
                return result == VK_SUCCESS;
        }

        void descriptor_pool::freeDescriptorSets(const std::vector<VkDescriptorSet>& sets) noexcept
        {
                if (m_pool == VK_NULL_HANDLE || sets.empty())
                        return;
                vkFreeDescriptorSets(m_device, m_pool, static_cast<uint32_t>(sets.size()), sets.data());
        }

        void descriptor_pool::moveFrom(descriptor_pool&& other) noexcept
        {
                m_device = other.m_device;
                m_pool   = other.m_pool;
                other.reset();
        }

        void descriptor_pool::reset() noexcept
        {
                m_device = VK_NULL_HANDLE;
                m_pool   = VK_NULL_HANDLE;
        }

        pipeline_layout::~pipeline_layout() {}

        pipeline_layout::pipeline_layout(pipeline_layout&& other) noexcept
        {
                moveFrom(std::move(other));
        }

        pipeline_layout& pipeline_layout::operator=(pipeline_layout&& other) noexcept
        {
                if (this != &other)
                {
                        destroy();
                        moveFrom(std::move(other));
                }
                return *this;
        }

        bool pipeline_layout::create(VkDevice device,
                                     const std::vector<std::vector<VkDescriptorSetLayoutBinding>>& bindingsPerSet,
                                     const std::vector<VkPushConstantRange>& pushConstants,
                                     uint32_t maxPersistentDescriptorSets)
        {
                if (device == VK_NULL_HANDLE)
                        return false;
                destroy();

                m_device = device;

                m_setLayouts.clear();
                m_setLayouts.reserve(bindingsPerSet.size());
                for (const auto& bindings : bindingsPerSet)
                {
                        VkDescriptorSetLayout layout = VK_NULL_HANDLE;
                        if (!createDescriptorSetLayout(device, bindings, layout))
                        {
                                for (auto l : m_setLayouts)
                                        vkDestroyDescriptorSetLayout(device, l, nullptr);
                                m_setLayouts.clear();
                                return false;
                        }
                        m_setLayouts.push_back(layout);
                }

                // create pipeline layout
                VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
                pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                pipelineLayoutInfo.setLayoutCount         = static_cast<uint32_t>(m_setLayouts.size());
                pipelineLayoutInfo.pSetLayouts            = m_setLayouts.empty() ? nullptr : m_setLayouts.data();

                pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstants.size());
                pipelineLayoutInfo.pPushConstantRanges    = pushConstants.empty() ? nullptr : pushConstants.data();

                if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
                {
                        for (auto l : m_setLayouts)
                                vkDestroyDescriptorSetLayout(device, l, nullptr);
                        m_setLayouts.clear();
                        m_pipelineLayout = VK_NULL_HANDLE;
                        return false;
                }

                // // Optionally create an internal descriptor pool for persistent sets
                // if (maxPersistentDescriptorSets > 0 && !m_setLayouts.empty())
                // {
                //         // Best-effort: build pool sizes from bindings across all sets
                //         std::vector<VkDescriptorPoolSize> poolSizes;
                //         // accumulate counts per descriptor type
                //         std::array<uint32_t, VK_DESCRIPTOR_TYPE_RANGE_SIZE>
                //             counts{}; // VK_DESCRIPTOR_TYPE_RANGE_SIZE is not standard; instead use a map
                //         // Simpler: iterate and append sizes for each binding; VK allows duplicate types in poolSizes
                //         for (const auto& bindings : bindingsPerSet)
                //         {
                //                 return true;
                //         }
                // }
                return true;
        }

        void pipeline_layout::destroy() noexcept
        {
                if (m_pipelineLayout != VK_NULL_HANDLE && m_device != VK_NULL_HANDLE)
                {
                        vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
                }
                m_pipelineLayout = VK_NULL_HANDLE;

                for (auto l : m_setLayouts)
                {
                        if (l != VK_NULL_HANDLE && m_device != VK_NULL_HANDLE)
                        {
                                vkDestroyDescriptorSetLayout(m_device, l, nullptr);
                        }
                }
                m_setLayouts.clear();

                if (m_descriptorPool)
                {
                        m_descriptorPool->destroy();
                        m_descriptorPool.reset();
                }

                m_device = VK_NULL_HANDLE;
        }

        bool pipeline_layout::allocateDescriptorSets(std::vector<VkDescriptorSet>& outSets)
        {
                if (!m_descriptorPool || m_descriptorPool->get() == VK_NULL_HANDLE)
                        return false;
                if (m_setLayouts.empty())
                        return false;

                return m_descriptorPool->allocateDescriptorSets(m_setLayouts, outSets);
        }

        bool pipeline_layout::createDescriptorSetLayout(VkDevice device,
                                                        const std::vector<VkDescriptorSetLayoutBinding>& bindings,
                                                        VkDescriptorSetLayout& outLayout)
        {
                if (device == VK_NULL_HANDLE)
                        return false;

                VkDescriptorSetLayoutCreateInfo layoutInfo{};
                layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
                layoutInfo.pBindings    = bindings.data();

                VkResult res            = vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &outLayout);
                if (res != VK_SUCCESS)
                {
                        outLayout = VK_NULL_HANDLE;
                        return false;
                }
                return true;
        }

        void pipeline_layout::moveFrom(pipeline_layout&& other) noexcept
        {
                m_device         = other.m_device;
                m_setLayouts     = std::move(other.m_setLayouts);
                m_pipelineLayout = other.m_pipelineLayout;
                m_descriptorPool = std::move(other.m_descriptorPool);

                other.reset();
        }

        void pipeline_layout::reset() noexcept
        {
                m_device         = VK_NULL_HANDLE;
                m_pipelineLayout = VK_NULL_HANDLE;
                m_setLayouts.clear();
                m_descriptorPool.reset();
        }

} // namespace ic
