#ifndef SHADER_H
#define SHADER_H

#include "defines.h"

#include <vulkan/vulkan.h>

namespace ic
{
class shader
{
private:
        VkDevice m_device;
        VkShaderModule m_shaderModule;

public:
        shader() = default;
        ~shader();

        shader(VkDevice device, const std::string &filename);

        shader(const shader &)            = delete;
        shader &operator=(const shader &) = delete;
        shader(shader &&other) noexcept;
        shader &operator=(shader &&other) noexcept;

        bool create(VkDevice device, const std::string &filename);
        void destroy();

        VkShaderModule getModule() const { return m_shaderModule; }

        operator VkShaderModule() const { return m_shaderModule; }

private:
        std::vector<char> readFile(const std::string &filename);
        void moveFrom(shader &&other) noexcept;
        void reset() noexcept;
};
}  // namespace ic

#endif
