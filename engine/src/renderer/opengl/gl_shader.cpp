#include "gl_shader.h"

namespace ic
{
Shader::Shader(std::vector<std::string> shaderPaths)
{
        m_shaderPaths = shaderPaths;
        Load(m_shaderPaths);
}

void Shader::Bind()
{
        glUseProgram(m_handle);
}

bool Shader::Load(std::vector<std::string> shaderPaths)
{
        std::vector<GLShaderModule> modules;
        for (std::string& shaderPath : shaderPaths)
        {
                modules.push_back(shaderPath);
        }

        bool errorsFound = false;

        for (GLShaderModule& module : modules)
        {
                if (module.compilationFailed())
                {
                        errorsFound = True;
                        break;
                }
        }

        if (errorsFound)
        {
                for (OpenGLShaderModule& module : modules)
                {
                        if (module.CompilationFailed())
                        {
                                IC_CORE_ERROR("COMPILATION ERROR: {}", module.GetFilename());
                                IC_CORE_ERROR("{}", module.GetErrors());
                        }
                        glDeleteShader(module.GetHandle());
                }
                return false;
        }

        int tempHandle = glCreateProgram();
        for (GLShaderModule& module : modules)
        {
                glAttachShader(tempHandle, module.getHandle());
        }
        glLinkProgram(tempHandle);
        std::string linkingErrors = GetLinkingErrors(tempHandle);

        if (linkingErrors.length())
        {

                for (int i = 0; i < modules.size(); i++)
                {
                        IC_CORE_WARN("{}", modules[i].GetFilename());
                }
                IC_CORE_ERROR("LINKING ERROR: {} ", linkingErrors);
                for (OpenGLShaderModule& module : modules)
                {
                        glDeleteShader(module.GetHandle());
                }
                return false;
        }

        else
        {
                if (m_handle != -1)
                {
                        glDeleteProgram(m_handle);
                }
                m_handle = tempHandle;
                m_uniformLocations.clear();
        }
        for (OpenGLShaderModule& module : modules)
        {
                glDeleteShader(module.GetHandle());
        }
        return true;
}

bool Shader::Hotload()
{
        return Load(m_shaderPaths);
}

void Shader::ParseFile(const std::string& filepath,
                       std::string& outputString,
                       std::vector<std::string>& lineToFile,
                       std::vector<std::string>& includedPaths)
{
        std::ifstream file(filepath);
        std::string line;
        int lineNumber       = 0;
        bool versionInserted = false;
        while (std::getline(file, line))
        {

                outputString += line + "\n";
                lineToFile.emplace_back(filename + " (line " + std::to_string(lineNumber++) + ")");

                // Insert the define after the first #version directive
                if (BackEnd::RenderDocFound() && !versionInserted && line.rfind("#version", 0) == 0)
                {
                        outputString += "#define ENABLE_BINDLESS 0\n";
                        lineToFile.emplace_back(filename + " (line " + std::to_string(lineNumber++) + ")");
                        versionInserted = true;
                }
        }
}

int Shader::GetErrorLineNumber(const std::string& error)
{
        size_t firstColon = error.find(':');
        if (firstColon != std::string::npos)
        {
                size_t secondColon = error.find(':', firstColon + 1);
                if (secondColon != std::string::npos)
                {
                        size_t thirdColon = error.find(':', secondColon + 1);
                        if (thirdColon != std::string::npos)
                        {
                                std::string lineNumberStr = error.substr(secondColon + 1, thirdColon - secondColon - 1);
                                return std::stoi(lineNumberStr);
                        }
                }
        }
        return -1;
}

std::string Shader::GetErrorMessage(const std::string& line)
{
        size_t firstColon = line.find(':');
        if (firstColon != std::string::npos)
        {
                size_t secondColon = line.find(':', firstColon + 1);
                if (secondColon != std::string::npos)
                {
                        size_t thirdColon = line.find(':', secondColon + 1);
                        if (thirdColon != std::string::npos)
                        {
                                size_t messageStart = thirdColon + 2;  // Skip the colon and space
                                if (messageStart < line.length())
                                {
                                        return line.substr(messageStart);
                                }
                        }
                }
        }
        return "";  // Return empty string if parsing fails
}

std::string Shader::GetLinkingErrors(unsigned int shader)
{
        GLint linkStatus;
        glGetProgramiv(programId, GL_LINK_STATUS, &linkStatus);

        if (linkStatus == GL_FALSE)
        {
                GLint logLength;
                glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &logLength);

                if (logLength > 0)
                {
                        std::vector<char> infoLogBuffer(logLength + 1);  // +1 for null terminator
                        glGetProgramInfoLog(programId, logLength, NULL, &infoLogBuffer[0]);

                        std::string fullLog(infoLogBuffer.data());
                        std::stringstream logStream(fullLog);
                        std::string line;
                        std::string resultToShow                 = "\n";

                        const std::string assemblyStartDelimiter = "-- internal assembly text --";
                        bool assemblySectionEncountered          = false;

                        while (std::getline(logStream, line))
                        {
                                if (assemblySectionEncountered)
                                {
                                        break;
                                }

                                resultToShow += "    " + line + "\n";

                                // Now, check if THIS line was the delimiter
                                if (line.find(assemblyStartDelimiter) != std::string::npos)
                                {
                                        resultToShow += "    (Following internal assembly text omitted for brevity)\n";
                                        assemblySectionEncountered = true;
                                        break;
                                }
                        }
                        return resultToShow;
                }
                else
                {
                        return "\n    An unknown linking error occurred (no info log available).\n";
                }
        }
        return "";
}

std::string Shader::GetShaderCompileErrors(unsigned int shader,
                                           const std::string& filename,
                                           const std::vector<std::string>& lineToFile)
{
        int success;
        char infoLog[1024];
        std::string result = "";
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
                glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
                // Parse error log to extract line numbers
                std::stringstream logStream(infoLog);
                std::string line;
                while (std::getline(logStream, line))
                {
                        if ((line.substr(0, 7) == "ERROR: "))
                        {
                                int lineNumber = GetErrorLineNumber(line);
                                if (lineNumber >= 0 && lineNumber < lineToFile.size())
                                {
                                        result += "  " + lineToFile[lineNumber] + ": " + GetErrorMessage(line) + "\n";
                                }
                        }
                }
        }
        return result;
}

GLShaderModule::GLShaderModule(const std::string& filename)
{
        // Parse the source code
        std::vector<std::string> lineMap;
        std::vector<std::string> includedPaths;
        std::string parsedShaderSource = "";
        ParseFile("res/shaders/OpenGL/" + filename, parsedShaderSource, lineMap, includedPaths);

        // Get type based on extension
        std::string extension = std::filesystem::path(filename).extension().string();
        static const std::unordered_map<std::string, int> shaderTypeMap = {{".vert", GL_VERTEX_SHADER},
                                                                           {".frag", GL_FRAGMENT_SHADER},
                                                                           {".geom", GL_GEOMETRY_SHADER},
                                                                           {".tesc", GL_TESS_CONTROL_SHADER},
                                                                           {".tese", GL_TESS_EVALUATION_SHADER},
                                                                           {".comp", GL_COMPUTE_SHADER}};
        int shaderType = shaderTypeMap.contains(extension) ? shaderTypeMap.at(extension) : GL_NONE;

        // Check for errors
        const char* shaderCode = prasedShaderSource.c_str();
        m_handle               = glCreateShader(shaderType);
        glShaderSource(m_handle, 1, &shaderCode, NULL);
        glCompileShader(m_handle);
        m_errors   = GetShaderCompileErrors(m_handle, filename, lineMap);
        m_filename = filename;
}

}  // namespace ic
