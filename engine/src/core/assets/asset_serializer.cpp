#include "core/assets/asset_serializer.h"
#include "core/filesystem.h"

/** TODO: Use my implementation here. */

namespace ic
{

Serializer::~Serializer()
{
        close();
}

bool Serializer::openForRead(const string &fname)
{
        IC_CORE_ASSERT(!isOpen(), "Dont forget to close last file used");
        filename = fname;
        if (!memMappedFile.open(fname, MemoryMapped::WholeFile, MemoryMapped::SequentialScan))
        {
                return false;
        }
        currentReadPos = memMappedFile.getData();

        return true;
}

bool Serializer::openForWrite(const string &fname)
{
        IC_CORE_ASSERT(!isOpen(), "Dont forget to close last file used");
        filename         = fname;
        char *parentPath = fs_getParentPath(fname.c_str());  // TODO
        if (!parentPath && !fs_exists(parentPath))
        {
                fs_mkdir(parentPath);
        }
        writeFile.open(fname, std::ios::binary);
        if (!writeFile || !writeFile.is_open())
        {
                IC_CORE_ERROR("Could not open file '%s'", fname.c_str());
                return false;
        }

        return true;
}

void Serializer::close()
{
        if (memMappedFile.isValid())
        {
                memMappedFile.close();
                currentReadPos = nullptr;
        }
        else if (writeFile.is_open())
        {
                writeFile.close();
        }
}

bool Serializer::isOpen() const
{
        return writeFile.is_open() || memMappedFile.isValid();
}

size_t Serializer::bytesLeft() const
{
        if (memMappedFile.isValid())
        {
                size_t bytesRead = currentReadPos - memMappedFile.getData();
                return memMappedFile.size() - bytesRead;
        }

        return 0;
}

const uint8_t *Serializer::getData() const
{
        IC_CORE_ASSERT(memMappedFile.isValid(), "Serialized file was not valid!");
        return currentReadPos;
}

void Serializer::write(const void *buffer, size_t bytes)
{
        IC_CORE_ASSERT(writeFile.good() && (buffer || (!buffer && !bytes)), "File written was not proper!");
        writeFile.write(reinterpret_cast<const char *>(buffer), bytes);
}

void Serializer::read(void *buffer, size_t bytes)
{
        IC_CORE_ASSERT(!bytes || (buffer && currentReadPos), "Bytes or current Position was NULL");
        IC_CORE_ASSERT(!bytes || (currentReadPos - memMappedFile.getData() + bytes <= memMappedFile.size()),
                       "Reading off the end of the file");
        memcpy(buffer, currentReadPos, bytes);
        currentReadPos += bytes;
}

void Serializer::skip(size_t bytes)
{
        IC_CORE_ASSERT(!bytes || (currentReadPos - memMappedFile.getData() + bytes <= memMappedFile.size()),
                       "Skipping off the end of the file");
        currentReadPos += bytes;
}

}  // namespace ic