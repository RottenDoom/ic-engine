#include "core/assets/asset_serializer.h"
#include "core/filesystem.h"

namespace ic
{

// ---------------------------------------------------------------------------
// Open / close
// ---------------------------------------------------------------------------

bool Serializer::openForRead(const char *fname)
{
        IC_CORE_ASSERT(!isOpen(), "Serializer: close the current file before opening another");

        m_filename           = fname;
        const char *fullpath = fs_getfullpath(fname);

        if (!mmap_open(&m_mmap, fullpath, MMAP_SEQUENTIAL))
        {
                IC_CORE_ERROR("Serializer: failed to memory-map '{}' (errno={})", fname, errno);
                ic_free((void *)fullpath);
                return false;
        }

        m_readPos = static_cast<const uint8_t *>(m_mmap.data);
        ic_free((void *)fullpath);
        return true;
}

bool Serializer::openForWrite(const char *fname)
{
        IC_CORE_ASSERT(!isOpen(), "Serializer: close the current file before opening another");
        IC_CORE_ASSERT(fname, "Serializer: empty filename");

        m_filename = fname;

        char *parentPath = fs_getParentPath(fname);
        if (parentPath && !fs_exists(parentPath))
                fs_mkdir(parentPath);
        ic_free(parentPath);

        // TODO: replace with fs_open
        m_writeFile.open(fname, std::ios::binary | std::ios::trunc);
        if (!m_writeFile.is_open())
        {
                IC_CORE_ERROR("Serializer: failed to open '{}' for writing", fname);
                return false;
        }
        return true;
}

void Serializer::close()
{
        if (mmap_valid(&m_mmap))
        {
                mmap_close(&m_mmap);
                m_readPos = nullptr;
                // mmap_close zeros the struct - mmap_valid() will return false
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

size_t Serializer::bytesLeft() const
{
        if (!mmap_valid(&m_mmap) || !m_readPos)
                return 0;

        size_t consumed = static_cast<size_t>(m_readPos - static_cast<const uint8_t *>(m_mmap.data));

        return static_cast<size_t>(m_mmap.file_size) - consumed;
}

const uint8_t *Serializer::getData() const
{
        IC_CORE_ASSERT(mmap_valid(&m_mmap), "Serializer::getData - not open for reading");
        return m_readPos;
}

void Serializer::read(void *buffer, size_t bytes)
{
        if (bytes == 0)
                return;

        IC_CORE_ASSERT(buffer, "Serializer::read - null buffer");
        IC_CORE_ASSERT(m_readPos, "Serializer::read - not open for reading");
        IC_CORE_ASSERT((m_readPos - static_cast<const uint8_t *>(m_mmap.data)) + bytes <= m_mmap.file_size,
                       "Serializer::read - {} bytes requested at offset {} would exceed file size {} ('{}')",
                       bytes,
                       tell(),
                       m_mmap.file_size,
                       m_filename.c_str());

        memcpy(buffer, m_readPos, bytes);
        m_readPos += bytes;
}

void Serializer::write(const void *buffer, size_t bytes)
{
        if (bytes == 0)
                return;

        IC_CORE_ASSERT(m_writeFile.is_open(), "Serializer::write - not open for writing");
        IC_CORE_ASSERT(buffer, "Serializer::write - null buffer");
        IC_CORE_ASSERT(m_writeFile.good(), "Serializer::write - stream in bad state");

        m_writeFile.write(reinterpret_cast<const char *>(buffer), static_cast<std::streamsize>(bytes));
}

void Serializer::skip(size_t bytes)
{
        if (bytes == 0)
                return;

        IC_CORE_ASSERT(m_readPos, "Serializer::skip - not open for reading");
        IC_CORE_ASSERT((m_readPos - static_cast<const uint8_t *>(m_mmap.data)) + bytes <= m_mmap.file_size,
                       "Serializer::skip - {} bytes would exceed file size",
                       bytes);

        m_readPos += bytes;
}

void Serializer::writeString(const char *s)
{
        uint32_t len = static_cast<uint32_t>(strlen(s));
        write(&len, sizeof(len));
        if (len > 0)
                write(s, len);
}

void Serializer::readString(std::string &out)
{
        uint32_t len = 0;
        read(&len, sizeof(len));

        // Same guard as readVector - a corrupt offset produces a garbage length
        IC_CORE_ASSERT(len <= 65536,
                       "Serializer::readString - length {} is impossibly large "
                       "(offset={} file='{}'). Field order mismatch?",
                       len,
                       tell(),
                       m_filename.c_str());

        out.resize(len);
        if (len > 0)
                read(out.data(), len);
}

}  // namespace ic