#ifndef ASSET_SERIALIZER_H
#define ASSET_SERIALIZER_H

#include "defines.h"
#include "core/filesystem.h"
#include "core/mmapped.h"

#include <fstream>
#include <string>
#include <cstring>
#include <cstdint>
#include <type_traits>

/**
 * asset_serializer.h
 *
 * Binary serializer for fast .icache asset files.
 *
 * Two modes -> mutually exclusive per instance:
 *   Read  -> memory-mapped via Mmap (C struct, zero-copy reads)
 *   Write -> std::ofstream in binary mode
 *
 * Usage:
 *   // Write
 *   Serializer s;
 *   s.openForWrite("assets/cache/mesh.icache");
 *   s.writePOD(header);
 *   s.writeBlob(vertices.data(), vertices.size() * sizeof(Vertex));
 *   s.close();
 *
 *   // Read
 *   s.openForRead("assets/cache/mesh.icache");
 *   s.readPOD(header);
 *   s.read(vertices.data(), vertices.size() * sizeof(Vertex));
 *   s.close();
 *
 * Design rules:
 *   - Mmap is a plain C struct -> no methods, no references, pointers only.
 *   - writePOD / readPOD are templated helpers restricted to trivially
 *     copyable types so you never accidentally serialize a std::vector.
 *   - writeBlob / read handle raw byte spans.
 *   - No exceptions -> all errors go through IC_CORE_ASSERT or return bool.
 */

namespace ic
{

class Serializer
{
public:
        Serializer() = default;
        ~Serializer() { close(); }

        // Non-copyable -> owns file handles
        Serializer(const Serializer &)            = delete;
        Serializer &operator=(const Serializer &) = delete;

        // -----------------------------------------------------------------------
        // Open / close
        // -----------------------------------------------------------------------

        /** Open an existing file for reading via memory map. */
        bool openForRead(const string &fname);

        /**
         * Open (or create) a file for writing.
         * Creates parent directories if they do not exist.
         */
        bool openForWrite(const string &fname);

        /** Flush and close whichever mode is active. Safe to call if not open. */
        void close();

        bool   isOpen() const;
        bool   isReading() const { return mmap_valid(&m_mmap); }
        bool   isWriting() const { return m_writeFile.is_open(); }
        string getFilename() const { return m_filename; }

        // -----------------------------------------------------------------------
        // Read API  (only valid after openForRead)
        // -----------------------------------------------------------------------

        /** Copy bytes from the current read position into buffer. Advances position. */
        void read(void *buffer, size_t bytes);

        /** Skip bytes without copying. */
        void skip(size_t bytes);

        /** Read a trivially copyable value by value. */
        template <typename T>
        void readPOD(T &out)
        {
                static_assert(std::is_trivially_copyable_v<T>, "readPOD requires a trivially copyable type");
                read(&out, sizeof(T));
        }

        /** Resize a vector and read its contents from the stream. */
        template <typename T>
        void readVector(std::vector<T> &out, size_t count)
        {
                static_assert(std::is_trivially_copyable_v<T>, "readVector requires a trivially copyable element type");
                out.resize(count);
                if (count > 0)
                        read(out.data(), count * sizeof(T));
        }

        /** Returns pointer to the current read position without advancing. */
        const uint8_t *getData() const;

        /** Remaining bytes from current read position to end of file. */
        size_t bytesLeft() const;

        // -----------------------------------------------------------------------
        // Write API  (only valid after openForWrite)
        // -----------------------------------------------------------------------

        /** Write raw bytes from buffer. */
        void write(const void *buffer, size_t bytes);

        /** Write a trivially copyable value. */
        template <typename T>
        void writePOD(const T &val)
        {
                static_assert(std::is_trivially_copyable_v<T>, "writePOD requires a trivially copyable type");
                write(&val, sizeof(T));
        }

        /**
         * Write a count followed by the raw element data.
         * Read back with: readPOD(count); readVector(vec, count);
         */
        template <typename T>
        void writeVector(const std::vector<T> &vec)
        {
                static_assert(std::is_trivially_copyable_v<T>,
                              "writeVector requires a trivially copyable element type");
                uint64_t count = vec.size();
                write(&count, sizeof(count));
                if (count > 0)
                        write(vec.data(), count * sizeof(T));
        }

        /** Write a string as uint32_t length + raw chars (no null terminator). */
        void writeString(const std::string &s);

        /** Read a string previously written by writeString(). */
        void readString(std::string &out);

private:
        Mmap           m_mmap    = {};       // zeroed -> mmap_valid() returns false
        const uint8_t *m_readPos = nullptr;  // current position within m_mmap.data
        std::ofstream  m_writeFile;
        string         m_filename;
};

}  // namespace ic

#endif  // ASSET_SERIALIZER_H