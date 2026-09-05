#include "renderer/opengl/gl_shader.h"

static GLuint compile_shader(GLenum type, const char *code, const GLint length, const char *filepath)
{
        GLuint shader = glCreateShader(type);

        glShaderSource(shader, 1, &code, &length);
        glCompileShader(shader);

        GLint success = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (success == GL_FALSE)
        {
                GLint logLength = 0;
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

                string log(logLength, '\0');
                glGetShaderInfoLog(shader, logLength, NULL, log.data());

                const char *shaderType = "UNKNOWN";

                switch (type)
                {
                case GL_VERTEX_SHADER:
                        shaderType = "VERTEX";
                        break;

                case GL_FRAGMENT_SHADER:
                        shaderType = "FRAGMENT";
                        break;

                case GL_GEOMETRY_SHADER:
                        shaderType = "GEOMETRY";
                        break;
                }

                IC_CORE_ERROR("Shader compilation failed\n"
                              "File: {}\n"
                              "Type: {}\n"
                              "{}",
                              filepath,
                              shaderType,
                              log);
                glDeleteShader(shader);

                return 0;
        }

        return shader;
}

namespace ic
{

Shader::Shader(const char *vertexPath, const char *fragmentPath)
{

        /** TODO file read from my api */
        std::string vertexCode;
        std::string fragCode;

        std::ifstream vShaderFile;
        std::ifstream fShaderFile;

        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        try
        {
                vShaderFile.open(vertexPath);
                fShaderFile.open(fragmentPath);
                std::stringstream vShaderStream, fShaderStream;

                vShaderStream << vShaderFile.rdbuf();
                fShaderStream << fShaderFile.rdbuf();

                vertexCode = vShaderStream.str();
                fragCode   = fShaderStream.str();
        }
        catch (std::ifstream::failure &e)
        {
                IC_CORE_ERROR("Could not read shader source: {} and {}", vertexPath, fragmentPath);
                IC_CORE_ERROR("std::ifstream exception: {}", e.what());
        }

        GLuint vertex   = compile_shader(GL_VERTEX_SHADER, vertexCode.c_str(), vertexCode.size(), vertexPath);
        GLuint fragment = compile_shader(GL_FRAGMENT_SHADER, fragCode.c_str(), fragCode.size(), fragmentPath);

        // shader Program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);

        GLint success = GL_FALSE;
        glGetProgramiv(ID, GL_LINK_STATUS, &success);

        if (success == GL_FALSE)
        {
                GLint logLength = 0;
                glGetProgramiv(ID, GL_INFO_LOG_LENGTH, &logLength);

                string log(logLength, '\0');
                glGetProgramInfoLog(ID, logLength, NULL, log.data());

                IC_CORE_ERROR("Shader linking failed: {}", log);
        }

        IC_CORE_TRACE("Created shader program with ID: {}", ID);

        // delete the shaders as they're linked into our program now and no longer necessary
        glDeleteShader(vertex);
        glDeleteShader(fragment);
}

void Shader::use()
{
        glUseProgram(ID);
}

}  // namespace ic
