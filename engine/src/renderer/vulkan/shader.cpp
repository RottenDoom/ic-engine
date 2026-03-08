#include "shader.h"

namespace ic
{
        shader::~shader() {}

        shader::shader(VkDevice device, const std::string& filename) : m_device(device)
        {
                create(device, filename);
        }

        shader::shader(shader&& other) noexcept
        {
                moveFrom(std::move(other));
        }

        shader& shader::operator=(shader&& other) noexcept
        {
                if (this != &other)
                {
                        destroy();
                        moveFrom(std::move(other));
                }
                return *this;
        }

        bool shader::create(VkDevice device, const std::string& filename)
        {
                std::vector<char> code = readFile(filename);
                VkShaderModuleCreateInfo CI{};
                CI.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
                CI.codeSize = code.size();
                CI.pCode    = reinterpret_cast<const uint32_t*>(code.data());

                if (vkCreateShaderModule(device, &CI, nullptr, &m_shaderModule) != VK_SUCCESS)
                {
                        IC_CORE_ERROR("Failed to create shader module.");
                        return false;
                }

                return true;
        }

        void shader::destroy()
        {
                reset();
        }

        std::vector<char> shader::readFile(const std::string& filename)
        {
                std::ifstream file(filename, std::ios::ate | std::ios::binary);

                if (!file.is_open())
                {
                        IC_CORE_ERROR("Failed to open File");
                }

                size_t fileSize = (size_t)file.tellg();
                IC_CORE_TRACE("Filesize: {0}", fileSize);

                std::vector<char> buffer(fileSize);

                file.seekg(0);
                file.read(buffer.data(), fileSize);

                file.close();

                return buffer;
        }
        void shader::moveFrom(shader&& other) noexcept
        {
                this->m_shaderModule = other.m_shaderModule;
                this->m_device       = other.m_device;

                other.m_shaderModule = VK_NULL_HANDLE;
                other.m_device       = VK_NULL_HANDLE;

                other.reset();
        }

        void shader::reset() noexcept
        {
                IC_CORE_TRACE("Deleted Staging Shader Modules");
                vkDestroyShaderModule(m_device, m_shaderModule, nullptr);
                m_shaderModule = VK_NULL_HANDLE;
                m_device       = VK_NULL_HANDLE;
        }

}  // namespace ic
