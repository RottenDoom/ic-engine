#include "gl_shader.h"

namespace ic
{

Shader::Shader(const char* vertexPath, const char* fragmentPath)
{
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
        catch (std::ifstream::failure e)
        {
                std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
        }
        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragCode.c_str();

        unsigned int vertex, fragment;
        int success;
        char infoLog[512];

        // vertex Shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        // print compile errors if any
        glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
        if (!success)
        {
                glGetShaderInfoLog(vertex, 512, NULL, infoLog);
                IC_CORE_ERROR("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n {}", infoLog);
        };

        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        // print compile errors if any
        glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
        if (!success)
        {
                glGetShaderInfoLog(vertex, 512, NULL, infoLog);
                IC_CORE_ERROR("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n {}", infoLog);
        };

        // shader Program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        // print linking errors if any
        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if (!success)
        {
                glGetProgramInfoLog(ID, 512, NULL, infoLog);
                IC_CORE_ERROR("ERROR::SHADER::PROGRAM::LINKING_FAILED\n {}", infoLog);
        }

        // delete the shaders as they're linked into our program now and no longer necessary
        glDeleteShader(vertex);
        glDeleteShader(fragment);
}

void Shader::use()
{
        glUseProgram(ID);
}

}  // namespace ic
