#pragma once
#include "defines.h"

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

        class pipeline
        {
        private:
                VkRenderPass renderPass;
                VkDescriptorSetLayout descriptorSetLayout;
                VkPipelineLayout pipelineLayout;
                VkPipeline graphicsPipeline;
                std::vector<VkFramebuffer> swapChainFramebuffers;
                VkDescriptorPool descriptorPool;
                std::vector<VkDescriptorSet> descriptorSets;

        public:
                std::vector<char> readBuffer(const std::string& filename);
        };
} // namespace ic
