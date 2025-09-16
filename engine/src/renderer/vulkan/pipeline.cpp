#include "pipeline.h"

namespace ic
{
        std::vector<char> pipeline::readBuffer(const std::string& filename)
        {
                std::ifstream file(filename, std::ios::ate | std::ios::binary);

                if (!file.is_open())
                {
                        IC_CORE_ERROR("Failed to open File");
                }

                size_t fileSize = (size_t) file.tellg();
                IC_CORE_TRACE("Filesize: {0}", fileSize);

                std::vector<char> buffer(fileSize);

                file.seekg(0);
                file.read(buffer.data(), fileSize);

                file.close();

                return buffer;
        }
} // namespace ic
