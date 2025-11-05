#pragma once
#include "defines.h"
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

        struct pipeline_config
        {
                std::vector<VkDynamicState> dynamicStateEnables = {
                    VK_DYNAMIC_STATE_VIEWPORT,
                    VK_DYNAMIC_STATE_SCISSOR,
                    VK_DYNAMIC_STATE_LINE_WIDTH,
                };

                VkViewport viewport{};
                VkRect2D scissor{};
                VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
                VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
                VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
                VkPipelineRasterizationStateCreateInfo rasterizer{};
                VkPipelineViewportStateCreateInfo viewportState{};
                VkPipelineMultisampleStateCreateInfo multisampling{};
                VkPipelineDepthStencilStateCreateInfo depthStencil{};
                VkPipelineColorBlendStateCreateInfo colorBlending{};

                void create(const VkExtent2D& swapchainExtent);
        };
}  // namespace ic