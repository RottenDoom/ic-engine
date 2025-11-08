#include "vulkan_renderer.h"

#include "shader.h"
#include "vkutils/descriptors.h"

#include <glm/gtc/matrix_transform.hpp>

namespace ic
{
        VkVertexInputBindingDescription Vertex::vertexInputBindingDescription{};
        std::vector<VkVertexInputAttributeDescription> Vertex::vertexInputAttributeDescriptions{};
        VkPipelineVertexInputStateCreateInfo Vertex::pipelineVertexInputStateCreateInfo{};

        vulkan_renderer::vulkan_renderer(vulkan_context* pContext, vkdevice& device) noexcept
            : m_device(device), vertexBuffer(device.logicalDevice), indexBuffer(device.logicalDevice)
        {
                context       = pContext;
                depthFormat   = m_device.getSupportedDepthFormat(true);
                auto extent2d = pContext->getWindowExtent();

                m_swapChain   = std::make_unique<SwapChain>(device, extent2d);
                m_renderPass  = std::make_unique<RenderPass>();

                uniformBuffers.reserve(m_swapChain->imageCount());
                for (size_t i = 0; i < MAX_FRAME_IN_FLIGHT; ++i)
                {
                        uniformBuffers.emplace_back(m_device.logicalDevice);
                }
                descriptorSets.resize(MAX_FRAME_IN_FLIGHT);

                camera.type = Camera::CameraType::lookat;
                camera.setPosition(glm::vec3(0.0f, 0.0f, -1.0f));
                camera.setViewDirection(glm::vec3(0.0f, 0.0f, -5.0f),
                                        glm::vec3(0.0f, 0.0f, -1.0f));  // look into +ve z axiz
                // camera.setRotationSpeed(0.5f);
                camera.setPerspectiveProjection(45.0f, (float)extent2d.width / (float)extent2d.height, 0.1f, 256.0f);
                // camera.setOrientation(glm::vec3(0.50f, 0.45f, 0.f));
        }

        bool vulkan_renderer::init()
        {
                VkDevice device = m_device.logicalDevice;
                createSwapChain();
                createRenderPass();
                createCommandPool();
                createCommandBuffers(device);
                createSyncObjects(device);
                setupDepthStencil(device);
                // pipeline cache
                createFramebuffers(device);
                loadAssets();
                // createVertexBuffer();
                // createIndexedBuffer();
                prepareUniformBuffers();
                setupDescriptors(device);
                createGraphicsPipeline(device);
                m_prepared = true;
                return true;
        }

        void vulkan_renderer::onEvent(event& e)
        {
                // TODO: Input Events like move and mouse scrolled
                camera.onEvent(e);
                eventDispatcher dispatcher(e);
                dispatcher.dispatch<WindowResizedEvent>(BIND_EVENT(vulkan_renderer::onWindowResize));
        }

        void vulkan_renderer::onUpdate(float deltaTime)
        {
                // TODO: we need events here

                // resize

                // render
                camera.onUpdate(deltaTime);
        }

        void vulkan_renderer::render(float deltaTime)
        {
                if (!m_prepared)
                        return;

                // render
                prepareFrame(m_device.logicalDevice);
                onUpdate(deltaTime);
                updateUniformBuffers();
                submitFrame(m_device.logicalDevice);
        }

        void vulkan_renderer::loadAssets()
        {
                const uint32_t glTFLoadingFlags = vkLoad::FileLoadingFlags::PreTransformVertices |
                                                  vkLoad::FileLoadingFlags::PreMultiplyVertexColors |
                                                  vkLoad::FileLoadingFlags::FlipY;

                scene.loadFromFile("treasure_smooth.gltf", &m_device, m_device.queues.transfer.handle, glTFLoadingFlags);
        }

        VkPipelineShaderStageCreateInfo vulkan_renderer::loadShader(std::string fileName, VkShaderStageFlagBits stage)
        {
                shader s = shader(m_device.logicalDevice, fileName);
                VkPipelineShaderStageCreateInfo ShaderStageCreateInfo{};
                ShaderStageCreateInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                ShaderStageCreateInfo.stage  = stage;

                ShaderStageCreateInfo.module = s.getModule();
                ShaderStageCreateInfo.pName  = "main";

                m_shaderModules.push_back(s.getModule());  // can move or copy errors

                return ShaderStageCreateInfo;
        }

        void vulkan_renderer::setupDescriptors(VkDevice& device)
        {

                VkDescriptorPoolSize descriptorPoolSize{};
                descriptorPoolSize.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorPoolSize.descriptorCount = MAX_FRAME_IN_FLIGHT;

                // pool
                std::vector<VkDescriptorPoolSize> poolSizes = {
                    descriptorPoolSize,
                };

                VkDescriptorPoolCreateInfo descriptorCI{};
                descriptorCI.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                descriptorCI.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
                descriptorCI.pPoolSizes    = poolSizes.data();
                descriptorCI.maxSets       = MAX_FRAME_IN_FLIGHT;

                IC_CORE_ASSERT(vkCreateDescriptorPool(device, &descriptorCI, nullptr, &descriptorPool) == VK_SUCCESS,
                               "Failed to Create Descriptor Pool");

                // layout
                VkDescriptorSetLayoutBinding UBOLayoutBinding{};
                UBOLayoutBinding = ic::descriptor::createDescriptorSetLayoutBinding(0,
                                                                                    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                                                    VK_SHADER_STAGE_VERTEX_BIT);

                std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
                    UBOLayoutBinding,
                };

                VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
                descriptorSetLayoutCI.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                descriptorSetLayoutCI.pBindings    = setLayoutBindings.data();
                descriptorSetLayoutCI.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
                IC_CORE_ASSERT(vkCreateDescriptorSetLayout(
                                   device, &descriptorSetLayoutCI, nullptr, &descriptorSetLayout) == VK_SUCCESS,
                               "Failed to create descriptor set layouts!");

                // sets per frame
                VkDescriptorSetAllocateInfo allocInfo{};
                allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                allocInfo.descriptorPool     = descriptorPool;
                allocInfo.pSetLayouts        = &descriptorSetLayout;
                allocInfo.descriptorSetCount = 1;

                for (size_t i = 0; i < uniformBuffers.size(); i++)
                {
                        IC_CORE_ASSERT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets[i]) == VK_SUCCESS,
                                       "Descriptor Set allocation failed!");
                        // Binding 0 : Vertex shader uniform buffer
                        VkWriteDescriptorSet writeDescriptorSet{};
                        writeDescriptorSet =
                            ic::descriptor::writeDescriptorSet(descriptorSets[i],
                                                               0,
                                                               0,
                                                               VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                               (const void*)(&uniformBuffers[i].descriptor));

                        std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
                            writeDescriptorSet,
                        };
                        vkUpdateDescriptorSets(device,
                                               static_cast<uint32_t>(writeDescriptorSets.size()),
                                               writeDescriptorSets.data(),
                                               0,
                                               nullptr);
                }

                IC_CORE_TRACE("Descriptors Setup Successful!");
        }

        void vulkan_renderer::createSwapChain()
        {
                IC_CORE_ASSERT(m_swapChain != nullptr, "SwapChain was not Initialized");
                IC_CORE_INFO("Swapchain created successfully!");
        }

        void vulkan_renderer::createRenderPass()
        {
                if (!m_renderPass->create(m_device.logicalDevice,
                                          m_swapChain->getSwapChainImageFormat(),
                                          m_device.getSupportedDepthFormat(true)))
                {
                        IC_CORE_ERROR("Failed to create Renderpass!");
                }
                IC_CORE_INFO("Renderpass created successfully!");
        }

        void vulkan_renderer::createCommandPool()
        {
                VkDevice device = m_device.logicalDevice;

                VkCommandPoolCreateInfo cmdPoolInfo{
                    .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                    .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                    .queueFamilyIndex = m_device.queues.graphics.index,
                };
                IC_CORE_ASSERT(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &cmdPool) == VK_SUCCESS,
                               "Failed to create Command Pool!");
                IC_CORE_TRACE("Command Pool Created!");
        }

        void vulkan_renderer::createCommandBuffers(VkDevice& device)
        {
                drawCmdBuffers.resize(MAX_FRAME_IN_FLIGHT);
                VkCommandBufferAllocateInfo cmdBufAllocateInfo{
                    .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                    .commandPool        = cmdPool,
                    .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                    .commandBufferCount = static_cast<uint32_t>(drawCmdBuffers.size()),
                };
                IC_CORE_ASSERT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, drawCmdBuffers.data()) ==
                                   VK_SUCCESS,
                               "Failed to allocate Draw Command Buffers");
                IC_CORE_TRACE("Command Buffers Created!");
        }

        void vulkan_renderer::createVertexBuffer()
        {
                VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

                // Create staging buffer
                buffer stagingBuffer(m_device.logicalDevice);
                m_device.createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                      &stagingBuffer,
                                      bufferSize,
                                      (void*)vertices.data());
                m_device.createBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                      &vertexBuffer,
                                      bufferSize);
                VkBufferCopy copyRegion{};
                copyRegion.size = bufferSize;
                m_device.copyBuffer(&stagingBuffer, &vertexBuffer, m_device.queues.graphics.handle, &copyRegion);
                stagingBuffer.destroy();
                IC_CORE_TRACE("Vertex Buffers Created");
        }

        void vulkan_renderer::createIndexedBuffer()
        {
                VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

                buffer stagingBuffer(m_device.logicalDevice);

                m_device.createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                      &stagingBuffer,
                                      bufferSize,
                                      (void*)indices.data());

                m_device.createBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                      &indexBuffer,
                                      bufferSize);

                VkBufferCopy copyRegion{};
                copyRegion.size = bufferSize;
                m_device.copyBuffer(&stagingBuffer, &indexBuffer, m_device.queues.graphics.handle, &copyRegion);
                stagingBuffer.destroy();
                IC_CORE_TRACE("Index Buffers Created");
        }

        void vulkan_renderer::createGraphicsPipeline(VkDevice& device)
        {
                VkPipelineLayoutCreateInfo layoutCI{};
                layoutCI.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                layoutCI.pSetLayouts    = &descriptorSetLayout;
                layoutCI.setLayoutCount = 1;

                IC_CORE_ASSERT(vkCreatePipelineLayout(device, &layoutCI, nullptr, &pipelineLayout) == VK_SUCCESS,
                               "Failed to set pipeline layout");

                config.create(context->getWindowExtent());

                VkGraphicsPipelineCreateInfo CI{};
                CI.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
                CI.layout              = pipelineLayout;
                CI.renderPass          = m_renderPass->get();
                CI.flags               = 0;
                CI.basePipelineIndex   = -1;
                CI.basePipelineHandle  = VK_NULL_HANDLE;

                CI.pInputAssemblyState = &config.inputAssembly;
                CI.pRasterizationState = &config.rasterizer;
                CI.pColorBlendState    = &config.colorBlending;
                CI.pMultisampleState   = &config.multisampling;
                CI.pViewportState      = &config.viewportState;
                CI.pDepthStencilState  = &config.depthStencil;
                CI.pVertexInputState   = ic::Vertex::getPipelineVertexInputState(
                    {ic::VertexComponent::Position, ic::VertexComponent::Normal, ic::VertexComponent::Color});
                CI.pDynamicState = &config.dynamicStateInfo;
                // This allows for derivatives of pipeline with this pipeline as base
                CI.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;

                std::vector<VkPipelineShaderStageCreateInfo> shaderStages(2);
                shaderStages[0] = loadShader("pipelines/phong.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
                shaderStages[1] = loadShader("pipelines/phong.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

                CI.stageCount   = static_cast<uint32_t>(shaderStages.size());
                CI.pStages      = shaderStages.data();

                VkPipelineCacheCreateInfo cacheCreateInfo{};
                cacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

                IC_CORE_ASSERT(vkCreatePipelineCache(device, &cacheCreateInfo, nullptr, &pipelineCache) == VK_SUCCESS,
                               "Failed to create Pipeline Cache!");

                if (vkCreateGraphicsPipelines(device, pipelineCache, 1, &CI, nullptr, &pipelines.phong) != VK_SUCCESS)
                {
                        IC_CORE_ERROR("Failed to create Graphics Pipeline");
                }

                // All pipelines created after the base pipeline will be derivatives
                CI.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
                // Base pipeline will be our first created pipeline
                CI.basePipelineHandle = pipelines.phong;
                // It's only allowed to either use a handle or index for the base pipeline
                // As we use the handle, we must set the index to -1 (see section 9.5 of the specification)
                CI.basePipelineIndex = -1;

                // Toon shading pipeline
                shaderStages[0] = loadShader("pipelines/toon.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
                shaderStages[1] = loadShader("pipelines/toon.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

                CI.stageCount   = static_cast<uint32_t>(shaderStages.size());
                CI.pStages      = shaderStages.data();

                if (vkCreateGraphicsPipelines(device, pipelineCache, 1, &CI, nullptr, &pipelines.toon) != VK_SUCCESS)
                {
                        IC_CORE_ERROR("Failed to create Graphics Pipeline");
                }

                // Pipeline for wire frame rendering
                // Non solid rendering is not a mandatory Vulkan feature
                if (enabledFeatures.fillModeNonSolid)
                {
                        config.rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
                        shaderStages[0] = loadShader("pipelines/wireframe.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
                        shaderStages[1] = loadShader("pipelines/wireframe.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

                        CI.stageCount   = static_cast<uint32_t>(shaderStages.size());
                        CI.pStages      = shaderStages.data();

                        if (vkCreateGraphicsPipelines(device, pipelineCache, 1, &CI, nullptr, &pipelines.wireFrame) !=
                            VK_SUCCESS)
                        {
                                IC_CORE_ERROR("Failed to create Graphics Pipeline");
                        }
                }

                IC_CORE_TRACE("Pipeline Creation Successfull!");
        }

        void vulkan_renderer::setupDepthStencil(VkDevice& device)
        {
                IC_CORE_ASSERT((depthFormat != VK_FORMAT_UNDEFINED), "Depth Format Invalid");
                VkImageCreateInfo imageCI{.sType       = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                                          .imageType   = VK_IMAGE_TYPE_2D,
                                          .format      = depthFormat,
                                          .extent      = {context->getWindowExtent().width,
                                                          context->getWindowExtent().height,
                                                          1},
                                          .mipLevels   = 1,
                                          .arrayLayers = 1,
                                          .samples     = VK_SAMPLE_COUNT_1_BIT,
                                          .tiling      = VK_IMAGE_TILING_OPTIMAL,
                                          .usage       = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT};
                IC_CORE_ASSERT(vkCreateImage(device, &imageCI, nullptr, &depthStencil.image) == VK_SUCCESS,
                               "Failed to depth stencil image");
                VkMemoryRequirements memReqs{};
                vkGetImageMemoryRequirements(device, depthStencil.image, &memReqs);

                VkMemoryAllocateInfo memAllloc{.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                                               .allocationSize  = memReqs.size,
                                               .memoryTypeIndex = m_device.getMemoryType(
                                                   memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};
                IC_CORE_ASSERT(vkAllocateMemory(device, &memAllloc, nullptr, &depthStencil.memory) == VK_SUCCESS,
                               "Couldn't allocate depth stencil memory");
                IC_CORE_ASSERT(vkBindImageMemory(device, depthStencil.image, depthStencil.memory, 0) == VK_SUCCESS,
                               "Couldn't Bind depth stencil image");

                VkImageViewCreateInfo imageViewCI{.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                                                  .image            = depthStencil.image,
                                                  .viewType         = VK_IMAGE_VIEW_TYPE_2D,
                                                  .format           = depthFormat,
                                                  .subresourceRange = {
                                                      .aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT,
                                                      .baseMipLevel   = 0,
                                                      .levelCount     = 1,
                                                      .baseArrayLayer = 0,
                                                      .layerCount     = 1,
                                                  }};
                // Stencil aspect should only be set on depth + stencil formats
                // (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT
                if (depthFormat >= VK_FORMAT_D16_UNORM_S8_UINT)
                {
                        imageViewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
                }
                IC_CORE_ASSERT(vkCreateImageView(device, &imageViewCI, nullptr, &depthStencil.view) == VK_SUCCESS,
                               "Failed to create depth stencil image view.");
                IC_CORE_TRACE("Depth Stencil Created!");
        }

        void vulkan_renderer::createFramebuffers(VkDevice& device)
        {
                swapchainFramebuffers.resize(m_swapChain->imageCount());

                IC_CORE_INFO("No of framebuffers: {}", swapchainFramebuffers.size());

                for (size_t i = 0; i < swapchainFramebuffers.size(); i++)
                {
                        VkImageView attachments[] = {m_swapChain->getImageView(i), depthStencil.view};
                        bool res                  = swapchainFramebuffers[i].create(
                            device, m_renderPass->get(), m_swapChain->getSwapChainExtent(), 2, attachments);

                        if (!res)
                        {
                                IC_CORE_WARN("Framebuffer was not created!");
                        }
                }
                IC_CORE_TRACE("Framebuffers Created!");
        }

        void vulkan_renderer::createSyncObjects(VkDevice& device)
        {
                imageAvailableSemaphores.resize(MAX_FRAME_IN_FLIGHT);
                renderFinishedSemaphores.resize(MAX_FRAME_IN_FLIGHT);  // this needs to be the same size as swapchain
                                                                       // image frames since this can differ
                inFlightFences.resize(MAX_FRAME_IN_FLIGHT);
                imagesInFlight.resize(m_swapChain->imageCount(), VK_NULL_HANDLE);

                for (uint32_t i = 0; i < MAX_FRAME_IN_FLIGHT; i++)
                {
                        // Fence used to ensure that command buffer has completed exection before using it again
                        VkFenceCreateInfo fenceCI{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
                        // Create the fences in signaled state (so we don't wait on first render of each command buffer)
                        fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;
                        IC_CORE_ASSERT(vkCreateFence(device, &fenceCI, nullptr, &inFlightFences[i]) == VK_SUCCESS,
                                       "Wait Fences creation Failed");
                }
                // Semaphores are used for correct command ordering within a queue
                // Used to ensure that image presentation is complete before starting to submit again
                for (auto& semaphore : imageAvailableSemaphores)
                {
                        VkSemaphoreCreateInfo semaphoreCI{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
                        IC_CORE_ASSERT(vkCreateSemaphore(device, &semaphoreCI, nullptr, &semaphore) == VK_SUCCESS,
                                       "Present Semaphore Creation Failed");
                }
                // Render completion
                // Semaphore used to ensure that all commands submitted have been finished before submitting the image
                // to the queue
                for (auto& semaphore : renderFinishedSemaphores)
                {
                        VkSemaphoreCreateInfo semaphoreCI{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
                        IC_CORE_ASSERT(vkCreateSemaphore(device, &semaphoreCI, nullptr, &semaphore) == VK_SUCCESS,
                                       "Render Semaphore Creation Failed");
                }

                IC_CORE_TRACE("Sync Objects Initialized!");
        }

        void vulkan_renderer::prepareFrame(VkDevice& device, bool waitForFence)
        {
                if (waitForFence)
                {
                        // Wait for command buffers to complete execution
                        IC_CORE_ASSERT(vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX) ==
                                           VK_SUCCESS,
                                       "Failed to wait for fences!");
                }

                VkResult result = vkAcquireNextImageKHR(device,
                                                        m_swapChain->getSwapChain(),
                                                        UINT64_MAX,
                                                        imageAvailableSemaphores[currentFrame],
                                                        VK_NULL_HANDLE,
                                                        &currentImageIndex);

                if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_frameBufferResized)
                {
                        m_frameBufferResized = false;
                        IC_CORE_TRACE("SwapChain Recreation Called!");
                        recreateSyncObjects();
                        recreateSwapChain();

                        // CRITICAL: Mark that we need to skip the submit for this frame
                        // because no image was successfully acquired
                        currentImageIndex = UINT32_MAX;  // Invalid index to signal skip
                        currentFrame      = (currentFrame + 1) % MAX_FRAME_IN_FLIGHT;
                        return;
                }

                IC_CORE_ASSERT(result == VK_SUCCESS, "Failed to Acquire SwapChain Images!");

                if (imagesInFlight[currentImageIndex] != VK_NULL_HANDLE)
                {
                        // Wait for the fence that is using this image to finish
                        vkWaitForFences(device, 1, &imagesInFlight[currentImageIndex], VK_TRUE, UINT64_MAX);
                }

                // 4) Mark this image as now being in use by currentFrame's fence
                imagesInFlight[currentImageIndex] = inFlightFences[currentFrame];

                // 5) Now record the command buffer for this image (reset + begin + record + end)
                // Make sure recordCommandBuffer resets it (vkResetCommandBuffer or vkResetCommandPool)
                buildCommandBuffers(currentImageIndex);  // you must implement this; example below
        }

        void vulkan_renderer::submitFrame(VkDevice& device, bool skipQueueSubmit)
        {
                // CRITICAL FIX: If no image was acquired (e.g., due to swapchain recreation),
                // skip the entire submission and presentation
                if (currentImageIndex == UINT32_MAX)
                {
                        // Advance to next frame slot even though we skipped
                        currentImageIndex = UINT32_MAX;
                        currentFrame      = (currentFrame + 1) % MAX_FRAME_IN_FLIGHT;
                        return;
                }

                vkResetFences(device, 1, &inFlightFences[currentFrame]);

                if (!skipQueueSubmit)
                {
                        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

                        VkSubmitInfo submitInfo{};
                        submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                        submitInfo.waitSemaphoreCount   = 1;
                        submitInfo.pWaitSemaphores      = &imageAvailableSemaphores[currentFrame];
                        submitInfo.pWaitDstStageMask    = waitStages;
                        submitInfo.commandBufferCount   = 1;
                        submitInfo.pCommandBuffers      = &drawCmdBuffers[currentImageIndex];
                        submitInfo.signalSemaphoreCount = 1;
                        submitInfo.pSignalSemaphores    = &renderFinishedSemaphores[currentFrame];

                        VkResult submitRes              = vkQueueSubmit(m_device.queues.graphics.handle,
                                                           1,
                                                           &submitInfo,
                                                           inFlightFences[currentFrame]);
                        IC_CORE_ASSERT(submitRes == VK_SUCCESS, "Queue Submission Failed!");
                }

                // Present: wait on the same render-finished semaphore for this frame
                VkPresentInfoKHR presentInfo{};
                presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
                presentInfo.waitSemaphoreCount = 1;
                presentInfo.pWaitSemaphores    = &renderFinishedSemaphores[currentFrame];

                VkSwapchainKHR swapchains[]    = {m_swapChain->getSwapChain()};
                presentInfo.swapchainCount     = 1;
                presentInfo.pSwapchains        = swapchains;
                presentInfo.pImageIndices      = &currentImageIndex;

                VkResult result                = vkQueuePresentKHR(m_device.queues.present.handle, &presentInfo);

                if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_frameBufferResized)
                {
                        m_frameBufferResized = false;
                        IC_CORE_TRACE("SwapChain Recreation Called!");
                        recreateSwapChain();
                }
                else if (result != VK_SUCCESS)
                {
                        IC_CORE_ASSERT(false, "Frame Submission Failed!");
                }

                // Advance to next frame slot
                currentFrame = (currentFrame + 1) % MAX_FRAME_IN_FLIGHT;
        }

        // used in renderframe
        void vulkan_renderer::buildCommandBuffers(uint32_t imageIndex)
        {
                VkCommandBuffer cmdBuffer = drawCmdBuffers[imageIndex];

                vkResetCommandBuffer(cmdBuffer, 0);

                VkCommandBufferBeginInfo cmdBufInfo{};
                cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                cmdBufInfo.flags = 0;  // or VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT if you require

                VkClearValue clearValues[2]{};
                clearValues[0].color        = defaultClearColor;
                clearValues[1].depthStencil = {1.0f, 0};

                VkRenderPassBeginInfo renderPassBeginInfo{};
                renderPassBeginInfo.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                renderPassBeginInfo.renderPass               = m_renderPass->get();
                renderPassBeginInfo.renderArea.offset.x      = 0;
                renderPassBeginInfo.renderArea.offset.y      = 0;
                renderPassBeginInfo.renderArea.extent.width  = context->getWindowExtent().width;
                renderPassBeginInfo.renderArea.extent.height = context->getWindowExtent().height;
                renderPassBeginInfo.clearValueCount          = 2;
                renderPassBeginInfo.pClearValues             = clearValues;
                renderPassBeginInfo.framebuffer              = swapchainFramebuffers[imageIndex];

                IC_CORE_ASSERT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo) == VK_SUCCESS,
                               "Command buffer begin failed");

                vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

                VkViewport viewport{};
                viewport.width    = (float)context->getWindowExtent().width;
                viewport.height   = (float)context->getWindowExtent().height;
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;
                vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

                VkRect2D scissor{};
                scissor.extent.width  = context->getWindowExtent().width;
                scissor.extent.height = context->getWindowExtent().height;
                scissor.offset.x      = 0;
                scissor.offset.y      = 0;
                vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

                vkCmdBindDescriptorSets(cmdBuffer,
                                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        pipelineLayout,
                                        0,
                                        1,
                                        &descriptorSets[currentFrame],
                                        0,
                                        nullptr);

                VkDeviceSize offsets[] = {0};
                vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.phong);
                vkCmdSetLineWidth(cmdBuffer, 1.0f);
                scene.draw(cmdBuffer);

                /* The scene and other things can be done later for now main target is to get something to show
                 * on screen.*/
                // scene.bindBuffers(cmdBuffer);

                // Left : Render the scene using the solid colored pipeline with phong shading
                // viewport.width = (float)extent2d.width / 3.0f;
                // vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
                // vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.phong);
                // vkCmdSetLineWidth(cmdBuffer, 1.0f);
                // scene.draw(cmdBuffer);

                // Center : Render the scene using a toon style pipeline
                // viewport.x = (float)width / 3.0f;
                // vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
                // vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.toon);
                // Line width > 1.0f only if wide lines feature is supported
                // if (enabledFeatures.wideLines)
                // {
                //         vkCmdSetLineWidth(cmdBuffer, 2.0f);
                // }
                // scene.draw(cmdBuffer);

                // Right : Render the scene as wireframe (if that feature is supported by the implementation)
                // if (enabledFeatures.fillModeNonSolid)
                // {
                //         viewport.x = (float)width / 3.0f + (float)width / 3.0f;
                //         vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
                //         vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.wireframe);
                //         scene.draw(cmdBuffer);
                // }

                // drawUI(cmdBuffer);

                vkCmdEndRenderPass(cmdBuffer);

                IC_CORE_ASSERT(vkEndCommandBuffer(cmdBuffer) == VK_SUCCESS, "Failed to end command buffer!");
        }

        void vulkan_renderer::prepareUniformBuffers()
        {
                for (auto& buffer : uniformBuffers)
                {
                        IC_CORE_ASSERT(m_device.createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                                             &buffer,
                                                             sizeof(UniformData)) == VK_SUCCESS,
                                       "Failed to Create Uniform Buffers");

                        IC_CORE_ASSERT(buffer.map() == VK_SUCCESS, "Buffer was mapped to CPU");
                }
        }

        // THis is a scene function and should be in a scene
        void vulkan_renderer::updateUniformBuffers()
        {
                UniformData ubo{};

                static auto startTime = std::chrono::high_resolution_clock::now();
                auto currentTime      = std::chrono::high_resolution_clock::now();
                float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

                glm::mat4 model       = glm::mat4(1.0f);

                float rotationSpeed   = glm::radians(45.0f);

                ubo.modelView         = model * camera.matrices.view;
                ubo.projection        = camera.projection;

                ubo.projection[1][1] *= -1;
                if (currentImageIndex != UINT32_MAX)
                        memcpy(uniformBuffers[currentImageIndex].mapped, &ubo, sizeof(UniformData));
                else
                {
                        IC_CORE_TRACE("Skipping Uniform Buffer Update");
                        return;
                }
        }

        // TODO: make sure swapchain extent dependencies are lowered
        void vulkan_renderer::recreateSwapChain()
        {
                vkDeviceWaitIdle(m_device.logicalDevice);
                // destroy the things
                for (auto& fb : swapchainFramebuffers)
                        vkDestroyFramebuffer(m_device.logicalDevice, fb, nullptr);
                destroyDepthStencil();

                m_swapChain->destroy(m_device.logicalDevice, nullptr);
                m_swapChain = nullptr;  // TODO : see how to fix this

                auto extent = context->getWindowExtent();
                while (extent.width == 0 || extent.height == 0)
                {
                        extent = context->getWindowExtent();
                        glfwWaitEvents();  // TODO: use own API
                }

                if (m_swapChain == nullptr)
                {
                        m_swapChain = std::make_unique<SwapChain>(m_device, extent);
                }
                else
                {
                        std::shared_ptr<SwapChain> oldSwapChain = std::move(m_swapChain);
                        m_swapChain = std::make_unique<SwapChain>(m_device, extent, oldSwapChain.get());
                }
                setupDepthStencil(m_device.logicalDevice);
                createFramebuffers(m_device.logicalDevice);

                // currentFrame = 0;
        }

        void vulkan_renderer::recreateSyncObjects()
        {
                VkDevice device = m_device.logicalDevice;

                // Destroy old semaphores for the current frame only
                vkDestroySemaphore(device, imageAvailableSemaphores[currentFrame], nullptr);
                vkDestroySemaphore(device, renderFinishedSemaphores[currentFrame], nullptr);

                // Recreate them
                VkSemaphoreCreateInfo semaphoreInfo{};
                semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

                IC_CORE_ASSERT(vkCreateSemaphore(device,
                                                 &semaphoreInfo,
                                                 nullptr,
                                                 &imageAvailableSemaphores[currentFrame]) == VK_SUCCESS,
                               "Failed to recreate image available semaphore!");

                IC_CORE_ASSERT(vkCreateSemaphore(device,
                                                 &semaphoreInfo,
                                                 nullptr,
                                                 &renderFinishedSemaphores[currentFrame]) == VK_SUCCESS,
                               "Failed to recreate render finished semaphore!");
        }

        void vulkan_renderer::destroyDepthStencil()
        {
                vkDestroyImageView(m_device.logicalDevice, depthStencil.view, nullptr);
                vkDestroyImage(m_device.logicalDevice, depthStencil.image, nullptr);
                vkFreeMemory(m_device.logicalDevice, depthStencil.memory, nullptr);
        }

        void vulkan_renderer::destroy()
        {
                VkDevice device = m_device.logicalDevice;
                vkDeviceWaitIdle(device);

                for (auto& buffer : uniformBuffers)
                {
                        buffer.destroy();
                }

                if (descriptorPool != VK_NULL_HANDLE)
                {
                        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
                }
                vkFreeCommandBuffers(device,
                                     cmdPool,
                                     static_cast<uint32_t>(drawCmdBuffers.size()),
                                     drawCmdBuffers.data());

                destroyDepthStencil();

                for (size_t i = 0; i < swapchainFramebuffers.size(); i++)
                {
                        swapchainFramebuffers[i].destroy(device);
                }
                swapchainFramebuffers.clear();

                for (auto& shaderModule : m_shaderModules)
                {
                        vkDestroyShaderModule(device, shaderModule, nullptr);
                }

                vkDestroyPipelineCache(device, pipelineCache, nullptr);
                vkDestroyCommandPool(device, cmdPool, nullptr);

                // delete everything else first
                for (size_t i = 0; i < imageAvailableSemaphores.size(); i++)
                {
                        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
                }
                for (size_t i = 0; i < renderFinishedSemaphores.size(); i++)
                {
                        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
                }
                for (uint32_t i = 0; i < MAX_FRAME_IN_FLIGHT; i++)
                {
                        vkDestroyFence(device, inFlightFences[i], nullptr);
                        vkDestroyBuffer(device, uniformBuffers[i].handle, nullptr);
                        vkFreeMemory(device, uniformBuffers[i].memory, nullptr);
                }
                scene.destroy(device);
                vertexBuffer.destroy();
                indexBuffer.destroy();

                vkDestroyPipeline(device, pipelines.phong, nullptr);
                if (enabledFeatures.fillModeNonSolid)
                {
                        vkDestroyPipeline(device, pipelines.wireFrame, nullptr);
                }
                vkDestroyPipeline(device, pipelines.toon, nullptr);
                vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
                vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

                m_renderPass->destroy(device, nullptr);
                m_swapChain->destroy(device, nullptr);

                IC_CORE_TRACE("Everything was destroyed!");
        }

        bool vulkan_renderer::onWindowResize(WindowResizedEvent& e)
        {
                if (e.getHeight() == 0 || e.getWidth() == 0)
                {
                        IC_CORE_TRACE("{0}, {1}", e.getHeight(), e.getWidth());
                        return true;  // TODO use better logic here.
                }
                m_frameBufferResized = true;
                return false;
        }

        void vulkan_renderer::getEnabledFeatures()
        {
                if (deviceFeatures.fillModeNonSolid)
                {
                        enabledFeatures.fillModeNonSolid = VK_TRUE;
                };

                if (deviceFeatures.wideLines)
                {
                        enabledFeatures.wideLines = VK_TRUE;
                }
        }
}  // namespace ic
