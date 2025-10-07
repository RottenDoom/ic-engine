#pragma once
#include "defines.h"
#include "pipeline_layout.h"
#include "shader.h"
#include <cstddef>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace ic
{
        /*
         * The graphics pipeline will be the main hub of all the rendering that should be done.
         * For now this API is going to be like a normal API like written in the vulkan tutorial but later on I plan
         * tp include API endpoints to tweak things from the editor. Making sure all that works first and I start
         * rendering and actually start to make my own game is the main goal in my own engine.
         * NOTE: I am going to add the buffer reading and shader reading logic somewhere else since I want all kinds of
         * shaders to be compatible (if possible)
         */

        struct vertex
        {
                glm::vec2 position;
                glm::vec3 color;

                inline static VkVertexInputBindingDescription getBindingDescription()
                {
                        VkVertexInputBindingDescription bindingDescription{};
                        bindingDescription.binding   = 0;
                        bindingDescription.stride    = sizeof(vertex);
                        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
                        return bindingDescription;
                }

                inline static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
                {
                        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
                        attributeDescriptions[0].binding  = 0;
                        attributeDescriptions[0].location = 0;
                        attributeDescriptions[0].format   = VK_FORMAT_R32G32_SFLOAT;
                        attributeDescriptions[0].offset   = offsetof(vertex, position);

                        attributeDescriptions[1].binding  = 0;
                        attributeDescriptions[1].location = 1;
                        attributeDescriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
                        attributeDescriptions[1].offset   = offsetof(vertex, color);
                        return attributeDescriptions;
                }

                inline static VkVertexInputAttributeDescription getColorAttributeDescription()
                {
                        VkVertexInputAttributeDescription attributeDescription{};
                        attributeDescription.binding  = 0;
                        attributeDescription.location = 1;
                        attributeDescription.format   = VK_FORMAT_R32G32B32_SFLOAT;
                        attributeDescription.offset   = offsetof(vertex, color);
                        return attributeDescription;
                }
        };
        struct pipeline_config
        {

                VkViewport viewport{};
                VkRect2D scissor{};
                std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
                VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
                VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
                VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
                VkPipelineRasterizationStateCreateInfo rasterizer{};
                VkPipelineViewportStateCreateInfo viewportState{};
                VkPipelineMultisampleStateCreateInfo multisampling{};
                std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
                VkPipelineColorBlendAttachmentState colorBlendAttachment{};
                VkPipelineDepthStencilStateCreateInfo depthStencil{};
                VkPipelineColorBlendStateCreateInfo colorBlending{};

                void create(const VkShaderModule& vertShaderModule, const VkShaderModule& fragShaderModule,
                            const VkExtent2D& swapchainExtent);
        };

        class pipeline
        {
        private:
                VkDevice m_device     = VK_NULL_HANDLE;
                VkPipeline m_pipeline = VK_NULL_HANDLE;

        public:
                pipeline() = default;
                ~pipeline();

                pipeline(const pipeline&)            = delete;
                pipeline& operator=(const pipeline&) = delete;
                pipeline(pipeline&& other) noexcept;
                pipeline& operator=(pipeline&& other) noexcept;

                bool create(VkDevice device, VkRenderPass renderPass, const pipeline_layout& layout,
                            const pipeline_config& config);

                void destroy();
                void bind(VkCommandBuffer cmd) const;

                VkPipeline get() const { return m_pipeline; }
                operator VkPipeline() const { return m_pipeline; }

        private:
                void moveFrom(pipeline&& other) noexcept;
                void reset() noexcept;
        };

} // namespace ic