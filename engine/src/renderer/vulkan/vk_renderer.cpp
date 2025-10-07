#include "vk_renderer.h"
#include "shader.h"

namespace ic
{
        void vulkan_renderer::moveFrom(vulkan_renderer&& other) noexcept
        {
                m_config                = std::move(other.m_config);
                m_layout                = std::move(other.m_layout);
                m_pipeline              = std::move(other.m_pipeline);
                m_swapchainFramebuffers = other.m_swapchainFramebuffers;

                // other handles
        }

        void vulkan_renderer::reset() noexcept
        {
                m_pipeline = nullptr;
                m_swapchainFramebuffers.clear();
        }

        vulkan_renderer::~vulkan_renderer() {}

        vulkan_renderer::vulkan_renderer(vulkan_renderer&& other) noexcept
        {
                moveFrom(std::move(other));
        }

        vulkan_renderer& vulkan_renderer::operator=(vulkan_renderer&& other) noexcept
        {
                if (this != &other)
                {
                        destroy(vulkan_context::get()->getDevice()->get());
                        moveFrom(std::move(other));
                }
                return *this;
        }

        // TODO: fix these gets everywhere bruh
        bool vulkan_renderer::create() noexcept
        {
                vulkan_context* context         = vulkan_context::get();
                const VkDevice device           = context->getDevice()->get();
                const VkPhysicalDevice phDevice = context->getDevice()->getPhysicalDevice()->get();
                const VkSurfaceKHR surface      = context->getSurface()->get();

                // Set 0 binding
                // TODO: use a enum or a struct to list out these bindings
                VkDescriptorSetLayoutBinding uboLayoutBinding{};
                uboLayoutBinding.binding            = 0;
                uboLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                uboLayoutBinding.descriptorCount    = 1;
                uboLayoutBinding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
                uboLayoutBinding.pImmutableSamplers = nullptr;

                std::vector<std::vector<VkDescriptorSetLayoutBinding>> bindingPerSet = {
                    {uboLayoutBinding},
                };

                m_layout.create(device, bindingPerSet); // TODO: add push constants maybe

                // Input the shader context (maybe make this info in the main render but for now this is okay since
                // I am just testing stuff)
                shader vert(device, "shader_scripts/bin/simple_shader.vert.spv");
                shader frag(device, "shader_scripts/bin/simple_shader.frag.spv");

                // create default configuration
                m_config.create(vert.getModule(), frag.getModule(), context->getSwapChain()->getExtent());

                // create pipeline
                m_pipeline = std::make_unique<pipeline>();
                if (!m_pipeline->create(device, context->getRenderpass()->get(), m_layout, m_config))
                {
                        IC_CORE_ERROR("Failed to create Pipeline!");
                        return false;
                }

                // create framebuffers (TODO: Make the framebuffer creation more real than this)
                createFramebuffers(context, context->getSwapChain()->getImageViews());

                m_cmdPool.create(device, phDevice, surface);
                m_cmdBuffers.allocate(device, m_cmdPool.commandPool, maxFrameInFlight);

                std::vector<VkDescriptorPoolSize> poolSizes = {
                    // TODO put this somewhere else
                    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxFrameInFlight}, // add more if you want
                };

                m_descriptorPool.create(device, poolSizes, maxFrameInFlight);
                m_descriptorPool.allocateDescriptorSets(m_layout.getDescriptorSetLayouts(), m_descriptorSets);

                m_fences.create(device, maxFrameInFlight);
                m_semaphores.create(device, maxFrameInFlight);

                return true;
        }

        void vulkan_renderer::drawFrame() noexcept
        {

                // need commandpool vertex index and uniform buffers, descriptor set and descriptor set
                // layout is done needs fences and semaphores classes

                IC_CORE_INFO("Draw call!");
        }

        void vulkan_renderer::destroy(const VkDevice& device)
        {
                vkDeviceWaitIdle(device);

                // destroybuffer

                // destroy buffer for index and uniform buffer

                m_fences.destroy(device);
                m_semaphores.destroy(device);

                m_cmdPool.destroy(device);
                for (auto* fb : m_swapchainFramebuffers)
                {
                        fb->destroy(device);
                        delete fb;
                }
                m_swapchainFramebuffers.clear();

                m_pipeline->destroy();
                m_layout.destroy();
                m_descriptorPool.destroy();
        }

        void vulkan_renderer::createFramebuffers(const vulkan_context* context,
                                                 const std::vector<VkImageView>& swapChainImageViews)
        {
                m_swapchainFramebuffers.resize(swapChainImageViews.size());

                IC_CORE_INFO("No of framebuffers: {}", m_swapchainFramebuffers.size());

                for (size_t i = 0; i < swapChainImageViews.size(); i++)
                {
                        VkImageView attachments[]  = {swapChainImageViews[i]};

                        m_swapchainFramebuffers[i] = new framebuffer();
                        bool res                   = m_swapchainFramebuffers[i]->create(
                            context->getDevice()->get(), context->getRenderpass()->get(),
                            context->getSwapChain()->getExtent(), 1, attachments);

                        if (!res)
                        {
                                IC_CORE_WARN("Framebuffer was not created!");
                        }
                }
        }

        void createVertexBuffers(logical_device& device, VkCommandPool& cmdPool, VkQueue& queue,
                                 const std::vector<vertex>& vertices, VkBuffer& buffer, VkDeviceMemory& memory)
        {
                VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

                VkBuffer stagingBuffer;
                VkDeviceMemory stagingBufferMemory;
                ibuffer::createBuffer(device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                      stagingBuffer, stagingBufferMemory);

                void* data;
                vkMapMemory(device.get(), stagingBufferMemory, 0, bufferSize, 0, &data);
                memcpy(data, vertices.data(), (size_t) bufferSize);
                vkUnmapMemory(device, stagingBufferMemory);

                ibuffer::createBuffer(device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                      buffer, memory);

                ibuffer::copyBuffer(device, cmdPool, queue, stagingBuffer, buffer, bufferSize);
                vkDestroyBuffer(device.get(), stagingBuffer, nullptr);
                vkFreeMemory(device.get(), stagingBufferMemory, nullptr);
        }

} // namespace ic
