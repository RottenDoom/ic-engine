#include "renderpass.h"

namespace ic
{
        render_pass::~render_pass() {}

        bool render_pass::create(VkDevice device, const VkFormat& swapchainImageForamt,
                                 const VkAllocationCallbacks* callback)
        {
                VkAttachmentDescription colorAttachment{};
                colorAttachment.format  = swapchainImageForamt; // TODO get this
                colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
                colorAttachment.loadOp =
                    VK_ATTACHMENT_LOAD_OP_CLEAR; // load, clear or don't care before and after rendering.
                colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE; // store or dont care (applies to color)
                colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // (applies to stencils)
                colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
                colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

                VkAttachmentReference ref{};
                ref.attachment = 0;
                ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                VkSubpassDescription subpass{};
                subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
                subpass.colorAttachmentCount = 1;
                subpass.pColorAttachments    = &ref;

                VkSubpassDependency deps{};
                deps.srcSubpass    = VK_SUBPASS_EXTERNAL;
                deps.dstSubpass    = 0;
                deps.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                deps.srcAccessMask = 0;
                deps.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                deps.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

                VkRenderPassCreateInfo CI{};
                CI.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
                CI.attachmentCount = 1;
                CI.pAttachments    = &colorAttachment;
                CI.subpassCount    = 1;
                CI.pSubpasses      = &subpass;
                CI.dependencyCount = 1;
                CI.pDependencies   = &deps;

                if (vkCreateRenderPass(device, &CI, callback, &m_renderPass) != VK_SUCCESS)
                {
                        IC_CORE_ERROR("Failed to create Render Pass!");
                        return false;
                }

                return true;
        }

        void render_pass::moveFrom(const render_pass&& other) noexcept
        {
                m_renderPass           = other.m_renderPass;
                m_swapchainImageFormat = other.m_swapchainImageFormat;
        }

        void render_pass::reset() noexcept
        {
                m_renderPass           = VK_NULL_HANDLE;
                m_swapchainImageFormat = VK_FORMAT_UNDEFINED;
        }

        void render_pass::destroy(VkDevice device, const VkAllocationCallbacks* callbacks)
        {
                if (m_renderPass != VK_NULL_HANDLE)
                {
                        vkDestroyRenderPass(device, m_renderPass, callbacks);
                        m_renderPass = VK_NULL_HANDLE;
                }
        }

        render_pass& render_pass::operator=(const render_pass&& other) noexcept
        {
                if (this != &other)
                {
                        m_renderPass           = VK_NULL_HANDLE;
                        m_swapchainImageFormat = other.m_swapchainImageFormat;
                }
                return *this;
        }

        render_pass::render_pass(const render_pass&& other) noexcept
        {
                moveFrom(std::move(other));
        }

} // namespace ic
