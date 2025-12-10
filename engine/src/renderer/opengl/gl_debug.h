#pragma once
#include "defines.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace ic
{

namespace gl::debug
{

GLenum glCheckError_(const char* file, int line)
{
        GLenum errorCode;
        while ((errorCode = glGetError()) != GL_NO_ERROR)
        {
                std::string error;
                switch (errorCode)
                {
                case GL_INVALID_ENUM:
                        error = "INVALID_ENUM";
                        break;
                case GL_INVALID_VALUE:
                        error = "INVALID_VALUE";
                        break;
                case GL_INVALID_OPERATION:
                        error = "INVALID_OPERATION";
                        break;
                case GL_STACK_OVERFLOW:
                        error = "STACK_OVERFLOW";
                        break;
                case GL_STACK_UNDERFLOW:
                        error = "STACK_UNDERFLOW";
                        break;
                case GL_OUT_OF_MEMORY:
                        error = "OUT_OF_MEMORY";
                        break;
                case GL_INVALID_FRAMEBUFFER_OPERATION:
                        error = "INVALID_FRAMEBUFFER_OPERATION";
                        break;
                }
                IC_CORE_ERROR("GL: ERROR: {} occured in file {}, at line {}", error, file, line);
        }
        return errorCode;
}

void APIENTRY glDebugOutput(GLenum source,
                            GLenum type,
                            unsigned int id,
                            GLenum severity,
                            GLsizei length,
                            const char* message,
                            const void* userParam)
{
        // ignore non-significant error/warning codes
        if (id == 131169 || id == 131185 || id == 131218 || id == 131204)
                return;

        IC_CORE_INFO("---------------");
        IC_CORE_INFO("GL Debug message: ({}), {}!", id, message);

        switch (source)
        {
        case GL_DEBUG_SOURCE_API:
                IC_CORE_INFO("Source: API");
                break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
                IC_CORE_INFO("Source: Window System");
                break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
                IC_CORE_INFO("Source: Shader Compiler");
                break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
                IC_CORE_INFO("Source: Third Party");
                break;
        case GL_DEBUG_SOURCE_APPLICATION:
                IC_CORE_INFO("Source: Application");
                break;
        case GL_DEBUG_SOURCE_OTHER:
                IC_CORE_INFO("Source: Other");
                break;
        }

        switch (type)
        {
        case GL_DEBUG_TYPE_ERROR:
                IC_CORE_WARN("Type: Error");
                break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
                IC_CORE_WARN("Type: Deprecated Behaviour");
                break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
                IC_CORE_WARN("Type: Undefined Behaviour");
                break;
        case GL_DEBUG_TYPE_PORTABILITY:
                IC_CORE_WARN("Type: Portability");
                break;
        case GL_DEBUG_TYPE_PERFORMANCE:
                IC_CORE_WARN("Type: Performance");
                break;
        case GL_DEBUG_TYPE_MARKER:
                IC_CORE_WARN("Type: Marker");
                break;
        case GL_DEBUG_TYPE_PUSH_GROUP:
                IC_CORE_WARN("Type: Push Group");
                break;
        case GL_DEBUG_TYPE_POP_GROUP:
                IC_CORE_WARN("Type: Pop Group");
                break;
        case GL_DEBUG_TYPE_OTHER:
                IC_CORE_WARN("Type: Other");
                break;
        }
        std::cout << std::endl;

        switch (severity)
        {
        case GL_DEBUG_SEVERITY_HIGH:
                IC_CORE_WARN("Severity: high");
                break;
        case GL_DEBUG_SEVERITY_MEDIUM:
                IC_CORE_WARN("Severity: medium");
                break;
        case GL_DEBUG_SEVERITY_LOW:
                IC_CORE_WARN("Severity: low");
                break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
                IC_CORE_WARN("Severity: notification");
                break;
        }
}

void setDebugOutput()
{
        int flags;
        glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
        if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
        {
                glEnable(GL_DEBUG_OUTPUT);
                glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
                glDebugMessageCallback(glDebugOutput, nullptr);
                // TODO: set severity using enum
                glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
                // glDebugMessageControl(
                //     GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_ERROR, GL_DEBUG_SEVERITY_HIGH, 0, nullptr, GL_TRUE);
        }
}

}  // namespace gl::debug

}  // namespace ic