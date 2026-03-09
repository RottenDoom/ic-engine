#include "core/assets/asset_serializer.h"
#include "core/filesystem.h"

namespace ic
{

// ---------------------------------------------------------------------------
// Open / close
// ---------------------------------------------------------------------------

bool Serializer::openForRead(const string &fname)
{
        IC_CORE_ASSERT(!isOpen(), "Serializer: close the current file before opening another");

        m_filename = fname;

        if (!mmap_open(&m_mmap, fname.c_str(), MMAP_SEQUENTIAL))
        {
                IC_CORE_ERROR("Serializer: failed to memory-map '{}'", fname.c_str());
                return false;
        }

        m_readPos = static_cast<const uint8_t *>(m_mmap.data);
        return true;
}

bool Serializer::openForWrite(const string &fname)
{
        IC_CORE_ASSERT(!isOpen(), "Serializer: close the current file before opening another");

        m_filename = fname;

        // Create parent directories if they don't exist
        char *parentPath = fs_getParentPath(fname.c_str());
        if (parentPath && !fs_exists(parentPath))
        {
                fs_mkdir(parentPath);
        }
        ic_free((void *)parentPath);

        // TODO: FIX THIS: USE MY OWN FS HERE
        m_writeFile.open(fname, std::ios::binary | std::ios::trunc);
        if (!m_writeFile.is_open())
        {
                IC_CORE_ERROR("Serializer: failed to open '{}' for writing", fname.c_str());
                return false;
        }

        return true;
}

void Serializer::close()
{
        if (mmap_valid(&m_mmap))
        {
                mmap_close(&m_mmap);
                // mmap_close zeros the struct internally -> but zero m_readPos too
                m_readPos = nullptr;
        }

        if (m_writeFile.is_open())
        {
                m_writeFile.flush();
                m_writeFile.close();
        }
}

bool Serializer::isOpen() const
{
        return mmap_valid(&m_mmap) || m_writeFile.is_open();
}

// ---------------------------------------------------------------------------
// Read API
// ---------------------------------------------------------------------------

void Serializer::read(void *buffer, size_t bytes)
{
        if (bytes == 0)
                return;

        IC_CORE_ASSERT(buffer, "Serializer::read -> null buffer");
        IC_CORE_ASSERT(m_readPos, "Serializer::read -> not open for reading");
        IC_CORE_ASSERT((m_readPos - static_cast<const uint8_t *>(m_mmap.data)) + bytes <= m_mmap.file_size,
                       "Serializer::read -> read would go past end of file");

        memcpy(buffer, m_readPos, bytes);
        m_readPos += bytes;
}

void Serializer::skip(size_t bytes)
{
        if (bytes == 0)
                return;

        IC_CORE_ASSERT(m_readPos, "Serializer::skip -> not open for reading");
        IC_CORE_ASSERT((m_readPos - static_cast<const uint8_t *>(m_mmap.data)) + bytes <= m_mmap.file_size,
                       "Serializer::skip -> skip would go past end of file");

        m_readPos += bytes;
}

const uint8_t *Serializer::getData() const
{
        IC_CORE_ASSERT(mmap_valid(&m_mmap), "Serializer::getData -> not open for reading");
        return m_readPos;
}

size_t Serializer::bytesLeft() const
{
        if (!mmap_valid(&m_mmap))
                return 0;

        size_t consumed = static_cast<size_t>(m_readPos - static_cast<const uint8_t *>(m_mmap.data));

        return static_cast<size_t>(m_mmap.file_size) - consumed;
}

// ---------------------------------------------------------------------------
// Write API
// ---------------------------------------------------------------------------

void Serializer::write(const void *buffer, size_t bytes)
{
        if (bytes == 0)
                return;

        IC_CORE_ASSERT(m_writeFile.is_open(), "Serializer::write -> not open for writing");
        IC_CORE_ASSERT(buffer, "Serializer::write -> null buffer");
        IC_CORE_ASSERT(m_writeFile.good(), "Serializer::write -> stream in bad state");

        m_writeFile.write(reinterpret_cast<const char *>(buffer), static_cast<std::streamsize>(bytes));
}

void Serializer::writeString(const std::string &s)
{
        uint32_t len = static_cast<uint32_t>(s.size());
        write(&len, sizeof(len));
        if (len > 0)
                write(s.data(), len);
}

void Serializer::readString(std::string &out)
{
        uint32_t len = 0;
        read(&len, sizeof(len));
        out.resize(len);
        if (len > 0)
                read(out.data(), len);
}

}  // namespace ic