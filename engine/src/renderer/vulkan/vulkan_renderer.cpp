#include "vulkan_renderer.h"
#include "shader.h"

namespace ic
{
        vulkan_renderer::vulkan_renderer(vulkan_context* pContext) noexcept
            : vertexBuffer(pContext->getVulkanDevice()->logicalDevice)
        {
                context     = pContext;
                depthFormat = context->getVulkanDevice()->getSupportedDepthFormat(true);

                vkGetDeviceQueue(context->getVulkanDevice()->logicalDevice,
                                 context->getVulkanDevice()->queueFamilyIndices.graphics,
                                 0,
                                 &queue);
                uniformBuffers.reserve(context->getSwapChain()->getImageCount());
                for (size_t i = 0; i < maxFrameInFlight; ++i)
                {
                        uniformBuffers.emplace_back(context->getVulkanDevice()->logicalDevice);  // TODO: fix this here
                }
                descriptorSets.resize(maxFrameInFlight);
        }

        bool vulkan_renderer::init()
        {
                VkDevice device = context->getVulkanDevice()->logicalDevice;
                createCommandPool();
                createCommandBuffers(device);
                createSyncObjects(device);
                setupDepthStencil(device);
                // pipeline cache
                createFramebuffers(device);
                loadAssets();
                createVertexBuffer();
                prepareUniformBuffers();
                setupDescriptors(device);
                createGraphicsPipeline(device);
                m_prepared = true;
                return true;
        }

        void vulkan_renderer::render()
        {
                if (!m_prepared)
                        return;
                prepareFrame(context->getVulkanDevice()->logicalDevice);
                updateUniformBuffers();
                buildCommandBuffers();
                submitFrame(context->getVulkanDevice()->logicalDevice);
        }

        void vulkan_renderer::loadAssets() {}

        void vulkan_renderer::setupDescriptors(VkDevice& device)
        {
                // pool
                VkDescriptorPoolSize descriptorPoolSize{.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                        .descriptorCount = maxFrameInFlight};
                std::vector<VkDescriptorPoolSize> poolSizes = {descriptorPoolSize};
                VkDescriptorPoolCreateInfo descriptorCI{};
                descriptorCI.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                descriptorCI.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
                descriptorCI.pPoolSizes    = poolSizes.data();
                descriptorCI.maxSets       = maxFrameInFlight;

                IC_CORE_ASSERT(vkCreateDescriptorPool(device, &descriptorCI, nullptr, &descriptorPool) == VK_SUCCESS,
                               "Failed to Create Descriptor Pool");

                // layout
                VkDescriptorSetLayoutBinding setLayoutBinding{};
                setLayoutBinding.descriptorType                             = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                setLayoutBinding.stageFlags                                 = VK_SHADER_STAGE_VERTEX_BIT;
                setLayoutBinding.binding                                    = 0;
                setLayoutBinding.descriptorCount                            = 1;

                std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
                    setLayoutBinding,
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
                        writeDescriptorSet.sType                              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        writeDescriptorSet.dstSet                             = descriptorSets[i];
                        writeDescriptorSet.descriptorType                     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                        writeDescriptorSet.dstBinding                         = 0;
                        writeDescriptorSet.pBufferInfo                        = &uniformBuffers[i].descriptor;
                        writeDescriptorSet.descriptorCount                    = 1;
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
        }  // namespace ic

        void vulkan_renderer::createCommandPool()
        {
                VkDevice device = context->getVulkanDevice()->logicalDevice;

                // TODO use the device version instead (write tests first)
                queue_family_indices queueFamilyIndices =
                    queue_manager::findQueueFamilies(context->getVulkanDevice()->physicalDevice, context->getSurface());

                VkCommandPoolCreateInfo cmdPoolInfo{
                    .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                    .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                    .queueFamilyIndex = queueFamilyIndices.graphicsFamily.value(),
                };
                IC_CORE_ASSERT(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &cmdPool) == VK_SUCCESS,
                               "Failed to create Command Pool!");
                IC_CORE_TRACE("Command Pool Created!");
        }

        void vulkan_renderer::createCommandBuffers(VkDevice& device)
        {
                drawCmdBuffers.resize(maxFrameInFlight);
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
                buffer stagingBuffer(context->getVulkanDevice()->logicalDevice);
                context->getVulkanDevice()->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                                         &stagingBuffer,
                                                         bufferSize,
                                                         (void*)vertices.data());
                context->getVulkanDevice()->createBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                                             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                         &vertexBuffer,
                                                         bufferSize);
                VkBufferCopy copyRegion{};
                copyRegion.size = bufferSize;
                context->getVulkanDevice()->copyBuffer(&stagingBuffer, &vertexBuffer, queue, &copyRegion);
                stagingBuffer.destroy();
                IC_CORE_TRACE("Vertex Buffers Created");
        }

        void vulkan_renderer::createGraphicsPipeline(VkDevice& device)
        {
                VkPipelineLayoutCreateInfo layoutCI{};
                layoutCI.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                layoutCI.pSetLayouts    = &descriptorSetLayout;
                layoutCI.setLayoutCount = 1;

                IC_CORE_ASSERT(vkCreatePipelineLayout(device, &layoutCI, nullptr, &pipelineLayout) == VK_SUCCESS,
                               "Failed to set pipeline layout");

                VkGraphicsPipelineCreateInfo CI{};
                CI.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
                CI.layout              = pipelineLayout;
                CI.renderPass          = context->getRenderpass()->get();
                CI.flags               = 0;
                CI.basePipelineIndex   = -1;
                CI.basePipelineHandle  = VK_NULL_HANDLE;

                CI.pInputAssemblyState = &config.inputAssembly;
                CI.pRasterizationState = &config.rasterizer;
                CI.pColorBlendState    = &config.colorBlending;
                CI.pMultisampleState   = &config.multisampling;
                CI.pViewportState      = &config.viewportState;
                CI.pDepthStencilState  = &config.depthStencil;
                CI.stageCount          = static_cast<uint32_t>(config.shaderStages.size());
                CI.pStages             = config.shaderStages.data();
                CI.pVertexInputState   = &config.vertexInputInfo;
                CI.pDynamicState       = &config.dynamicStateInfo;

                CI.flags               = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;

                VkPipelineCacheCreateInfo cacheCreateInfo{};
                cacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

                IC_CORE_ASSERT(vkCreatePipelineCache(device, &cacheCreateInfo, nullptr, &pipelineCache) == VK_SUCCESS,
                               "Failed to create Pipeline Cache!");

                // Make different types of pipeline based on enabled features
                // Phong shading pipeline
                shader phong_vert(device, "pipelines/phong.vert.spv");
                shader phong_frag(device, "pipelines/phong.frag.spv");

                auto bindingDescriptions  = Vertex::getBindingDescriptions();
                auto attributeDesciptions = Vertex::getAttributeDescriptions();
                config.create(phong_vert.getModule(),
                              phong_frag.getModule(),
                              extent2d,
                              bindingDescriptions,
                              attributeDesciptions);

                if (vkCreateGraphicsPipelines(device, pipelineCache, 1, &CI, nullptr, &pipelines.phong) != VK_SUCCESS)
                {
                        IC_CORE_ERROR("Failed to create Graphics Pipeline");
                }

                // subsequent pipelines are derivatives
                CI.flags              = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
                CI.basePipelineHandle = pipelines.phong;
                CI.basePipelineIndex  = -1;

                phong_frag.destroy();
                phong_vert.destroy();
                IC_CORE_TRACE("Pipeline Creation Successfull!");
        }

        void vulkan_renderer::setupDepthStencil(VkDevice& device)
        {
                IC_CORE_ASSERT((depthFormat != VK_FORMAT_UNDEFINED), "Depth Format Invalid");
                extent2d = context->getSwapChain()->getExtent();
                VkImageCreateInfo imageCI{.sType       = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                                          .imageType   = VK_IMAGE_TYPE_2D,
                                          .format      = depthFormat,
                                          .extent      = {extent2d.width, extent2d.height, 1},
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
                                               .memoryTypeIndex = context->getVulkanDevice()->getMemoryType(
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
                std::vector<VkImageView> swapchainImageViews = context->getSwapChain()->getImageViews();
                swapchainFramebuffers.resize(swapchainImageViews.size());

                IC_CORE_INFO("No of framebuffers: {}", swapchainFramebuffers.size());

                for (size_t i = 0; i < swapchainImageViews.size(); i++)
                {
                        VkImageView attachments[] = {swapchainImageViews[i], depthStencil.view};
                        bool res                  = swapchainFramebuffers[i].create(device,
                                                                   context->getRenderpass()->get(),
                                                                   context->getSwapChain()->getExtent(),
                                                                   2,
                                                                   attachments);

                        if (!res)
                        {
                                IC_CORE_WARN("Framebuffer was not created!");
                        }
                }
                IC_CORE_TRACE("Framebuffers Created!");
        }

        void vulkan_renderer::createSyncObjects(VkDevice& device)
        {
                presentSemaphores.resize(maxFrameInFlight);
                renderSemaphore.resize(maxFrameInFlight);  // this needs to be the same size as swapchain image frames
                                                           // since this can differ
                waitFences.resize(maxFrameInFlight);

                for (uint32_t i = 0; i < maxFrameInFlight; i++)
                {
                        // Fence used to ensure that command buffer has completed exection before using it again
                        VkFenceCreateInfo fenceCI{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
                        // Create the fences in signaled state (so we don't wait on first render of each command buffer)
                        fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;
                        IC_CORE_ASSERT(vkCreateFence(device, &fenceCI, nullptr, &waitFences[i]) == VK_SUCCESS,
                                       "Wait Fences creation Failed");
                }
                // Semaphores are used for correct command ordering within a queue
                // Used to ensure that image presentation is complete before starting to submit again
                for (auto& semaphore : presentSemaphores)
                {
                        VkSemaphoreCreateInfo semaphoreCI{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
                        IC_CORE_ASSERT(vkCreateSemaphore(device, &semaphoreCI, nullptr, &semaphore) == VK_SUCCESS,
                                       "Present Semaphore Creation Failed");
                }
                // Render completion
                // Semaphore used to ensure that all commands submitted have been finished before submitting the image
                // to the queue
                for (auto& semaphore : renderSemaphore)
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
                        // wait for command buffers to complete execution
                        IC_CORE_ASSERT(vkWaitForFences(device, 1, &waitFences[currentBuffer], VK_TRUE, UINT64_MAX) ==
                                           VK_SUCCESS,
                                       "Failed to wait for fences!");
                        IC_CORE_ASSERT(vkResetFences(device, 1, &waitFences[currentBuffer]) == VK_SUCCESS,
                                       "Failed to reset fences!");
                }

                // TODO maybe put this function inside swapchain
                VkResult result = vkAcquireNextImageKHR(device,
                                                        context->getSwapChain()->swapchainHandle,
                                                        UINT64_MAX,
                                                        presentSemaphores[currentBuffer],
                                                        VK_NULL_HANDLE,
                                                        &currentImageIndex);

                if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR))
                {
                        if (result == VK_ERROR_OUT_OF_DATE_KHR)
                        {
                                windowResize();
                        }
                        return;
                }
                else
                {
                        IC_CORE_ASSERT(result == VK_SUCCESS, "Frame Preparation Failed!");
                }
        }

        // TODO
        void vulkan_renderer::submitFrame(VkDevice& device, bool skipQueueSubmit)
        {
                if (!skipQueueSubmit)
                {
                        const VkPipelineStageFlags waitPipelineStage{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
                        VkSubmitInfo submitInfo{.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                                                .waitSemaphoreCount   = 1,
                                                .pWaitSemaphores      = &presentSemaphores[currentBuffer],
                                                .pWaitDstStageMask    = &waitPipelineStage,
                                                .commandBufferCount   = 1,
                                                .pCommandBuffers      = &drawCmdBuffers[currentBuffer],
                                                .signalSemaphoreCount = 1,
                                                .pSignalSemaphores    = &renderSemaphore[currentImageIndex]};
                        IC_CORE_ASSERT(vkQueueSubmit(queue, 1, &submitInfo, waitFences[currentBuffer]) == VK_SUCCESS,
                                       "Queue Submition Failed!");
                }

                VkPresentInfoKHR presentInfo{.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                                             .waitSemaphoreCount = 1,
                                             .pWaitSemaphores    = &renderSemaphore[currentImageIndex],
                                             .swapchainCount     = 1,
                                             .pSwapchains        = &context->getSwapChain()->swapchainHandle,
                                             .pImageIndices      = &currentImageIndex};
                VkResult result = vkQueuePresentKHR(queue, &presentInfo);
                // Recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE) or no longer
                // optimal for presentation (SUBOPTIMAL)
                if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR))
                {
                        windowResize();
                        if (result == VK_ERROR_OUT_OF_DATE_KHR)
                        {
                                return;
                        }
                }
                else
                {
                        IC_CORE_ASSERT(result == VK_SUCCESS, "Frame Submition Failed!");
                }
                // Select the next frame to render to, based on the max. no. of concurrent frames
                currentBuffer = (currentBuffer + 1) % maxFrameInFlight;
        }

        // used in renderframe
        void vulkan_renderer::buildCommandBuffers()
        {
                VkCommandBuffer cmdBuffer = drawCmdBuffers[currentBuffer];

                VkCommandBufferBeginInfo cmdBufInfo{};
                cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

                VkClearValue clearValues[2]{};
                clearValues[0].color        = defaultClearColor;
                clearValues[1].depthStencil = {1.0f, 0};

                VkRenderPassBeginInfo renderPassBeginInfo{};
                renderPassBeginInfo.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                renderPassBeginInfo.renderPass               = context->getRenderpass()->get();
                renderPassBeginInfo.renderArea.offset.x      = 0;
                renderPassBeginInfo.renderArea.offset.y      = 0;
                renderPassBeginInfo.renderArea.extent.width  = extent2d.width;
                renderPassBeginInfo.renderArea.extent.height = extent2d.height;
                renderPassBeginInfo.clearValueCount          = 2;
                renderPassBeginInfo.pClearValues             = clearValues;
                renderPassBeginInfo.framebuffer              = swapchainFramebuffers[currentImageIndex];

                IC_CORE_ASSERT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo) == VK_SUCCESS,
                               "Command buffer begin failed");

                vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

                VkViewport viewport{};
                viewport.width    = (float)extent2d.width;
                viewport.height   = (float)extent2d.height;
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;
                vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

                VkRect2D scissor{};
                scissor.extent.width  = extent2d.width;
                scissor.extent.height = extent2d.height;
                scissor.offset.x      = 0;
                scissor.offset.y      = 0;
                vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

                vkCmdBindDescriptorSets(cmdBuffer,
                                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        pipelineLayout,
                                        0,
                                        1,
                                        &descriptorSets[currentBuffer],
                                        0,
                                        nullptr);

                VkDeviceSize offsets[] = {0};
                vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.phong);
                vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer.handle, offsets);
                vkCmdDraw(cmdBuffer, 3, 1, 0, 0);

                /* The scene and other things can be done later for now main target is to get something to show on
                 * screen.*/
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
                        IC_CORE_ASSERT(context->getVulkanDevice()->createBuffer(
                                           VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                           &buffer,
                                           sizeof(UniformData)) == VK_SUCCESS,
                                       "Failed to Create Uniform Buffers");

                        IC_CORE_ASSERT(buffer.map() == VK_SUCCESS, "Buffer was mapped to CPU");
                }
        }

        void vulkan_renderer::updateUniformBuffers()
        {
                UniformData ubo{};
                ubo.modelView = glm::mat4(1.0f);
                ubo.projection =
                    glm::perspective(glm::radians(60.0f), extent2d.width / (float)extent2d.height, 0.1f, 256.0f);

                memcpy(uniformBuffers[currentImageIndex].mapped, &ubo, sizeof(ubo));
        }

        void vulkan_renderer::destroy()
        {
                VkDevice device = context->getVulkanDevice()->logicalDevice;
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

                for (size_t i = 0; i < swapchainFramebuffers.size(); i++)
                {
                        swapchainFramebuffers[i].destroy(device);
                }
                swapchainFramebuffers.clear();

                vkDestroyImageView(device, depthStencil.view, nullptr);
                vkDestroyImage(device, depthStencil.image, nullptr);
                vkFreeMemory(device, depthStencil.memory, nullptr);

                vkDestroyPipelineCache(device, pipelineCache, nullptr);
                vkDestroyCommandPool(device, cmdPool, nullptr);

                // delete everything else first
                for (size_t i = 0; i < presentSemaphores.size(); i++)
                {
                        vkDestroySemaphore(device, presentSemaphores[i], nullptr);
                }
                for (size_t i = 0; i < renderSemaphore.size(); i++)
                {
                        vkDestroySemaphore(device, renderSemaphore[i], nullptr);
                }
                for (uint32_t i = 0; i < maxFrameInFlight; i++)
                {
                        vkDestroyFence(device, waitFences[i], nullptr);
                        vkDestroyBuffer(device, uniformBuffers[i].handle, nullptr);
                        vkFreeMemory(device, uniformBuffers[i].memory, nullptr);
                }
                vertexBuffer.destroy();

                vkDestroyPipeline(device, pipelines.phong, nullptr);
                vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
                vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

                IC_CORE_TRACE("Everything was destroyed!");
        }

        void vulkan_renderer::windowResize() {}

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

        std::vector<VkVertexInputBindingDescription> vulkan_renderer::Vertex::getBindingDescriptions()
        {
                std::vector<VkVertexInputBindingDescription> bindingDescription(1);
                bindingDescription[0].binding   = 0;  // first binding
                bindingDescription[0].stride    = sizeof(Vertex);
                bindingDescription[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

                return bindingDescription;
        }
        std::vector<VkVertexInputAttributeDescription> vulkan_renderer::Vertex::getAttributeDescriptions()
        {
                std::vector<VkVertexInputAttributeDescription> attributeDescription(2);
                attributeDescription[0].binding  = 0;
                attributeDescription[0].location = 0;
                attributeDescription[0].offset   = offsetof(Vertex, pos);
                attributeDescription[0].format   = VK_FORMAT_R32G32B32_SFLOAT;

                attributeDescription[1].binding  = 0;
                attributeDescription[1].location = 1;
                attributeDescription[1].offset   = offsetof(Vertex, color);
                attributeDescription[1].format   = VK_FORMAT_R32G32B32_SFLOAT;

                return attributeDescription;
        }
}  // namespace ic
