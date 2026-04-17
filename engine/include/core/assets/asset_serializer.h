#ifndef ASSET_SERIALIZER_H
#define ASSET_SERIALIZER_H

#include "defines.h"
#include "core/mmapped.h"

#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <type_traits>

/**
 * asset_serializer.h - Binary serializer for .icmodel asset files.
 *
 * Two modes, mutually exclusive per instance:
 *   Read  - memory-mapped via Mmap (C struct, zero-copy)
 *   Write - std::ofstream binary
 *
 * -------------------------------------------------------------------------
 * SYMMETRY RULE - this is the source of most serialization bugs:
 *
 *   Every writeX() call must be matched by exactly one readX() call
 *   in the same position, reading the same number of bytes.
 *
 *   writePOD    <->  readPOD      sizeof(T) bytes
 *   writeVector <->  readVector   8-byte count + N*sizeof(T) bytes
 *   writeString <->  readString   4-byte length + N bytes
 *   write       <->  read         raw N bytes, caller manages count
 * -------------------------------------------------------------------------
 *
 * Use tell() to verify symmetry during debugging. After each write the
 * stream advances by a known amount. The same advance must happen on the
 * read side at the same call site. First divergence = bug location.
 */

namespace ic
{

class Serializer
{
public:
        Serializer() = default;
        ~Serializer() { close(); }

        Serializer(const Serializer &)            = delete;
        Serializer &operator=(const Serializer &) = delete;

        // -----------------------------------------------------------------------
        // Open / close
        // -----------------------------------------------------------------------

        bool openForRead(const string &fname);
        bool openForWrite(const string &fname);
        void close();

        bool   isOpen() const;
        bool   isReading() const { return mmap_valid(&m_mmap); }
        bool   isWriting() const { return m_writeFile.is_open(); }
        string getFilename() const { return m_filename; }

        // -----------------------------------------------------------------------
        // Stream position
        //
        // tell() returns the current byte offset from the start of the file.
        // Use it to bracket every read/write during debugging:
        //
        //   IC_CORE_TRACE("before meshCount @ {}", s->tell());
        //   s->writePOD(meshCount);
        //   IC_CORE_TRACE("after  meshCount @ {}", s->tell());
        //
        // The save-side trace and the load-side trace must print the same
        // offsets at the same logical positions. First mismatch = bug.
        // -----------------------------------------------------------------------

        size_t tell() const
        {
                if (mmap_valid(&m_mmap) && m_readPos)
                        return static_cast<size_t>(m_readPos - static_cast<const uint8_t *>(m_mmap.data));
                return 0;
        }

        size_t         bytesLeft() const;
        const uint8_t *getData() const;

        // -----------------------------------------------------------------------
        // Primitives
        // -----------------------------------------------------------------------

        void read(void *buffer, size_t bytes);
        void write(const void *buffer, size_t bytes);
        void skip(size_t bytes);

        // -----------------------------------------------------------------------
        // POD
        // -----------------------------------------------------------------------

        template <typename T>
        void writePOD(const T &val)
        {
                static_assert(std::is_trivially_copyable_v<T>, "writePOD: T must be trivially copyable");
                write(&val, sizeof(T));
        }

        template <typename T>
        void readPOD(T &out)
        {
                static_assert(std::is_trivially_copyable_v<T>, "readPOD: T must be trivially copyable");
                read(&out, sizeof(T));
        }

        // -----------------------------------------------------------------------
        // Vector  [uint64_t count | T * count]
        //
        // The count is always a uint64_t (8 bytes) on both sides.
        // writeVector and readVector are always used as a matched pair.
        // -----------------------------------------------------------------------

        template <typename T>
        void writeVector(const std::vector<T> &vec)
        {
                static_assert(std::is_trivially_copyable_v<T>, "writeVector: T must be trivially copyable");
                uint64_t count = static_cast<uint64_t>(vec.size());
                write(&count, sizeof(count));
                if (count > 0)
                        write(vec.data(), static_cast<size_t>(count) * sizeof(T));
        }

        template <typename T>
        void readVector(std::vector<T> &out)
        {
                static_assert(std::is_trivially_copyable_v<T>, "readVector: T must be trivially copyable");

                uint64_t count = 0;
                read(&count, sizeof(count));

                // A count this large is always a stream-offset bug, not a real asset.
                // Catches the "288230376151711744 bad_alloc" class of crash before
                // it reaches vector::resize. Limit is 4 GB of elements - no asset
                // will ever legitimately exceed this.
                IC_CORE_ASSERT(count <= 0x0000000100000000ULL,
                               "Serializer::readVector - count {} is impossibly large "
                               "(file='{}' offset={}). "
                               "Save/load field order mismatch or corrupt cache.",
                               count,
                               m_filename.c_str(),
                               tell());

                out.resize(static_cast<size_t>(count));
                if (count > 0)
                        read(out.data(), static_cast<size_t>(count) * sizeof(T));
        }

        // -----------------------------------------------------------------------
        // String  [uint32_t length | chars (no null terminator)]
        // -----------------------------------------------------------------------

        void writeString(const std::string &s);
        void readString(std::string &out);

private:
        Mmap           m_mmap    = {};  // zero-init - mmap_valid() returns false
        const uint8_t *m_readPos = nullptr;
        std::ofstream  m_writeFile;
        string         m_filename;
};

}  // namespace ic

#endif  // ASSET_SERIALIZER_H