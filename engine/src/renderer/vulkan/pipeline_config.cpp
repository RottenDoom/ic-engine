#include "pipeline_config.h"

namespace ic
{
        // TODO: make sure config struct creation gets split and configurable
        void pipeline_config::create(const VkShaderModule& vertShaderModule,
                                     const VkShaderModule& fragShaderModule,
                                     const VkExtent2D& swapchainExtent,
                                     const std::vector<VkVertexInputBindingDescription>& bindingDescriptions,
                                     const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions)
        {
                VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
                vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;

                vertShaderStageInfo.module = vertShaderModule;
                vertShaderStageInfo.pName  = "main";

                VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
                fragShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;

                fragShaderStageInfo.module = fragShaderModule;
                fragShaderStageInfo.pName  = "main";

                // shader stages
                shaderStages = {vertShaderStageInfo, fragShaderStageInfo};

                // create vertex input info
                vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
                vertexInputInfo.vertexBindingDescriptionCount   = static_cast<uint32_t>(bindingDescriptions.size());
                vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
                vertexInputInfo.pVertexBindingDescriptions      = bindingDescriptions.data();
                vertexInputInfo.pVertexAttributeDescriptions    = attributeDescriptions.data();

                // create input assembly info
                inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
                inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                inputAssembly.primitiveRestartEnable = VK_FALSE;

                /*
                 * viewport state should have its own struct wrapper however I am including this here to edit
                 * later when I think of changing
                 */

                viewport.x        = 0.0f;
                viewport.y        = 0.0f;
                viewport.width    = (float)swapchainExtent.width;
                viewport.height   = (float)swapchainExtent.height;
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;

                scissor.offset    = {0, 0};
                scissor.extent    = swapchainExtent;

                // dynamic state creation info
                dynamicStateInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
                dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
                dynamicStateInfo.pDynamicStates    = dynamicStates.data();

                // viewport state creation
                viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
                viewportState.viewportCount = 1;
                viewportState.pViewports    = &viewport;
                viewportState.scissorCount  = 1;
                viewportState.pScissors     = &scissor;

                // rasterizer creation info
                // it can be configured to output fragments that fill entire polygons or just the edges (wireframe
                // rendering).
                // set face culling and depth testiing here.
                rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
                rasterizer.cullMode                = VK_CULL_MODE_NONE;
                rasterizer.depthClampEnable        = VK_FALSE;
                rasterizer.rasterizerDiscardEnable = VK_FALSE;
                rasterizer.polygonMode = VK_POLYGON_MODE_FILL;  // this is responsible for different kind of modes for
                                                                // wireframes. requires gpu feature for other modes.
                                                                // LINE and POINt Fill
                rasterizer.lineWidth = 1.0f;
                // rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
                rasterizer.frontFace               = VK_FRONT_FACE_CLOCKWISE;
                rasterizer.depthBiasEnable         = VK_FALSE;
                rasterizer.depthBiasConstantFactor = 0.0f;  // Optional // used in shadowmapping
                rasterizer.depthBiasClamp          = 0.0f;  // Optional
                rasterizer.depthBiasSlopeFactor    = 0.0f;  // Optional // used in shadowmapping

                // multisampling creation
                // MSAA antialiasing options here.
                multisampling.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
                multisampling.sampleShadingEnable   = VK_FALSE;
                multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
                multisampling.minSampleShading      = 1.0f;      // Optional
                multisampling.pSampleMask           = nullptr;   // Optional
                multisampling.alphaToCoverageEnable = VK_FALSE;  // Optional
                multisampling.alphaToOneEnable      = VK_FALSE;  // Optional

                // depthStencil.sType  // skiping for now
                depthStencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
                depthStencil.depthTestEnable       = VK_TRUE;
                depthStencil.depthWriteEnable      = VK_TRUE;
                depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS;
                depthStencil.depthBoundsTestEnable = VK_FALSE;
                depthStencil.stencilTestEnable     = VK_FALSE;

                // Color blend attachment
                // blending can be done by mixing or taking the bitwise operation. We have one framebuffer
                colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                                      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
                colorBlendAttachment.blendEnable         = VK_FALSE;
                colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
                colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
                colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;       // Optional
                colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
                colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
                colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;       // Optional

                // the first way of blending is done by the following type of psudecode
                // if (blendEnable) {
                //     finalColor.rgb = (srcColorBlendFactor * newColor.rgb) <colorBlendOp> (dstColorBlendFactor *
                //     oldColor.rgb); finalColor.a = (srcAlphaBlendFactor * newColor.a) <alphaBlendOp>
                //     (dstAlphaBlendFactor * oldColor.a);
                // } else {
                //     finalColor = newColor;
                // }
                // finalColor = finalColor & colorWriteMask;

                // Most common way is alpha blending where colors are mixed based on there opacity
                // finalColor.rgb = newAlpha * newColor + (1 - newAlpha) * oldColor;
                // finalColor.a = newAlpha.a;

                // Can be done this way
                // colorBlendAttachment.blendEnable = VK_TRUE;
                // colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
                // colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                // colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
                // colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                // colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
                // colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

                // color blending info
                colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
                colorBlending.logicOpEnable   = VK_FALSE;
                colorBlending.logicOp         = VK_LOGIC_OP_COPY;  // Optional
                colorBlending.attachmentCount = 1;
                colorBlending.pAttachments =
                    &colorBlendAttachment;  // https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions
                colorBlending.blendConstants[0] = 0.0f;  // Optional
                colorBlending.blendConstants[1] = 0.0f;  // Optional
                colorBlending.blendConstants[2] = 0.0f;  // Optional
                colorBlending.blendConstants[3] = 0.0f;  // Optional
        }
}  // namespace ic
