#ifndef ASSET_SERIALIZER_H
#define ASSET_SERIALIZER_H

#include "defines.h"
#include <MemoryMapped.h>

namespace ic
{

class Serializer
{
public:
        Serializer() = default;
        ~Serializer();

        bool openForRead(const string &filename);
        bool openForWrite(const string &filename);
        void close();
        bool isOpen() const;

        // if opened for reading, return how many bytes are left to read. Else 0
        size_t bytesLeft() const;
        const uint8_t *getData() const;

        void write(const void *buffer, size_t bytes);
        void read(void *buffer, size_t bytes);
        void skip(size_t bytes);

        template <typename LenType = uint32_t>
        void write(const string &s)
        {
                LenType strSize = static_cast<LenType>(s.length());
                write(strSize);
                if (strSize > 0)
                {
                        write(&s[0], strSize);
                }
        }

        template <typename LenType = int32_t>
        void read(string &s)
        {
                LenType strSize;
                read(strSize);
                if (strSize > 0)
                {
                        s.resize(strSize);
                        read(&s[0], strSize);
                }
        }

        template <typename T>
        void write(const T &x)
        {
                static_assert(std::is_trivial<T>::value, "T must be a trivial plain old data type");
                write(&x, sizeof(T));
        }

        template <typename T>
        void write(const std::vector<T> &x)
        {
                static_assert(std::is_trivial<T>::value, "T must be a trivial plain old data type");
                size_t size = x.size();
                write(size);
                write(x.data(), x.size() * sizeof(T));
        }

        template <typename T>
        void write(const char *str)
        {
                T len = (T)strlen(str);
                write(len);
                write(str, len);
        }

        template <typename T>
        void read(T &x)
        {
                static_assert(!std::is_const<T>::value, "T must be non-const");
                static_assert(std::is_trivial<T>::value, "T must be a trivial plain old data type");
                read((void *)&x, sizeof(T));
        }

        template <typename T>
        void read(std::vector<T> &x)
        {
                static_assert(!std::is_const<T>::value, "T must be non-const");
                static_assert(std::is_trivial<T>::value, "T must be a trivial plain old data type");
                size_t size;
                read(size);
                x.resize(size);
                read((void *)x.data(), size * sizeof(T));
        }

        template <typename T>
        T read()
        {
                T val;
                read(val);
                return val;
        }

private:
        string filename;
        std::ofstream writeFile;
        MemoryMapped memMappedFile;
        unsigned char *currentReadPos = nullptr;
};

}  // namespace ic

#endif