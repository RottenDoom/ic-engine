#include "core/mmapped.h"
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

// ------------------------------------------------------------------ helpers --

static void mmap_zero(Mmap *m)
{
        memset(m, 0, sizeof(Mmap));
#if defined(_WIN32) || defined(_WIN64)
        m->file_handle    = INVALID_HANDLE_VALUE;
        m->mapping_handle = NULL;
#else
        m->fd = -1;
#endif
}

// ------------------------------------------------------------ page size ------

size_t mmap_page_size(void)
{
#if defined(_WIN32) || defined(_WIN64)
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        return (size_t)si.dwAllocationGranularity;  // use granularity not page size on windows
                                                    // MapViewOfFile offset must align to this
                                                    // (usually 65536, not 4096)
#else
        return (size_t)sysconf(_SC_PAGE_SIZE);
#endif
}

// ------------------------------------------------------------------ valid ----

bool mmap_valid(const Mmap *m)
{
        return m && m->data != NULL;
}

// ------------------------------------------------------------------ open -----

#if defined(_WIN32) || defined(_WIN64)

static bool win_open_file(Mmap *m, const char *path)
{
        m->file_handle =
            CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (m->file_handle == INVALID_HANDLE_VALUE)
                return false;

        LARGE_INTEGER size;
        if (!GetFileSizeEx(m->file_handle, &size))
        {
                CloseHandle(m->file_handle);
                m->file_handle = INVALID_HANDLE_VALUE;
                return false;
        }
        m->file_size = (uint64_t)size.QuadPart;
        return true;
}

static DWORD win_hint_flags(MmapHint hint)
{
        switch (hint)
        {
        case MMAP_SEQUENTIAL:
                return FILE_FLAG_SEQUENTIAL_SCAN;
        case MMAP_RANDOM:
                return FILE_FLAG_RANDOM_ACCESS;
        default:
                return FILE_ATTRIBUTE_NORMAL;
        }
}

static bool win_map_range(Mmap *m, uint64_t offset, size_t size)
{
        // create or recreate mapping object
        if (m->mapping_handle)
        {
                CloseHandle(m->mapping_handle);
                m->mapping_handle = NULL;
        }

        m->mapping_handle = CreateFileMapping(m->file_handle,
                                              NULL,
                                              PAGE_READONLY,
                                              0,
                                              0,  // map whole file in the mapping object
                                              NULL);
        if (!m->mapping_handle)
                return false;

        ULARGE_INTEGER off;
        off.QuadPart = offset;

        m->data = MapViewOfFile(m->mapping_handle,
                                FILE_MAP_READ,
                                off.HighPart,
                                off.LowPart,
                                size  // 0 would mean whole file, we pass explicit size
        );

        if (!m->data)
        {
                CloseHandle(m->mapping_handle);
                m->mapping_handle = NULL;
                return false;
        }

        m->map_size = size;
        m->offset   = offset;
        return true;
}

#else  // POSIX

static int posix_hint_advice(MmapHint hint)
{
        switch (hint)
        {
        case MMAP_SEQUENTIAL:
                return MADV_SEQUENTIAL;
        case MMAP_RANDOM:
                return MADV_RANDOM;
        default:
                return MADV_NORMAL;
        }
}

static bool posix_map_range(Mmap *m, uint64_t offset, size_t size, MmapHint hint)
{
        void *addr = mmap(NULL, size, PROT_READ, MAP_PRIVATE, m->fd, (off_t)offset);

        if (addr == MAP_FAILED)
                return false;

        // advise the kernel of our access pattern
        madvise(addr, size, posix_hint_advice(hint));

        m->data     = addr;
        m->map_size = size;
        m->offset   = offset;
        return true;
}

#endif

// ---------------------------------------------------------------- public api -

bool mmap_open(Mmap *m, const char *path, MmapHint hint)
{
        if (!m || !path)
                return false;
        mmap_zero(m);

#if defined(_WIN32) || defined(_WIN64)
        // reopen with hint flags baked in for whole-file open
        m->file_handle =
            CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, win_hint_flags(hint), NULL);
        if (m->file_handle == INVALID_HANDLE_VALUE)
                return false;

        LARGE_INTEGER size;
        if (!GetFileSizeEx(m->file_handle, &size))
        {
                CloseHandle(m->file_handle);
                mmap_zero(m);
                return false;
        }
        m->file_size = (uint64_t)size.QuadPart;

        if (m->file_size == 0)
        {
                // valid empty file, nothing to map
                return true;
        }

        return win_map_range(m, 0, (size_t)m->file_size);

#else
        m->fd = open(path, O_RDONLY);
        if (m->fd < 0)
                return false;

        struct stat st;
        if (fstat(m->fd, &st) < 0)
        {
                close(m->fd);
                mmap_zero(m);
                return false;
        }
        m->file_size = (uint64_t)st.st_size;

        if (m->file_size == 0)
                return true;

        return posix_map_range(m, 0, (size_t)m->file_size, hint);
#endif
}

bool mmap_open_range(Mmap *m, const char *path, uint64_t offset, size_t size, MmapHint hint)
{
        if (!m || !path)
                return false;
        mmap_zero(m);

        size_t page = mmap_page_size();
        if (offset % page != 0)
                return false;  // caller must align offset

#if defined(_WIN32) || defined(_WIN64)
        if (!win_open_file(m, path))
                return false;

        size_t clamped = (size == 0 || offset + size > m->file_size) ? (size_t)(m->file_size - offset) : size;

        return win_map_range(m, offset, clamped);
#else
        m->fd = open(path, O_RDONLY);
        if (m->fd < 0)
                return false;

        struct stat st;
        if (fstat(m->fd, &st) < 0)
        {
                close(m->fd);
                mmap_zero(m);
                return false;
        }
        m->file_size = (uint64_t)st.st_size;

        size_t clamped = (size == 0 || offset + size > m->file_size) ? (size_t)(m->file_size - offset) : size;

        if (!posix_map_range(m, offset, clamped, hint))
        {
                close(m->fd);
                mmap_zero(m);
                return false;
        }
        return true;
#endif
}

bool mmap_remap(Mmap *m, uint64_t offset, size_t size, MmapHint hint)
{
        if (!m)
                return false;

        size_t page = mmap_page_size();
        if (offset % page != 0)
                return false;

        // unmap current view but keep file handle open
#if defined(_WIN32) || defined(_WIN64)
        if (m->data)
        {
                UnmapViewOfFile(m->data);
                m->data = NULL;
        }
        size_t clamped = (offset + size > m->file_size) ? (size_t)(m->file_size - offset) : size;
        return win_map_range(m, offset, clamped);
#else
        if (m->data)
        {
                munmap(m->data, m->map_size);
                m->data = NULL;
        }
        size_t clamped = (offset + size > m->file_size) ? (size_t)(m->file_size - offset) : size;
        return posix_map_range(m, offset, clamped, hint);
#endif
}

void mmap_close(Mmap *m)
{
        if (!m)
                return;

#if defined(_WIN32) || defined(_WIN64)
        if (m->data)
                UnmapViewOfFile(m->data);
        if (m->mapping_handle)
                CloseHandle(m->mapping_handle);
        if (m->file_handle != INVALID_HANDLE_VALUE)
                CloseHandle(m->file_handle);
#else
        if (m->data)
                munmap(m->data, m->map_size);
        if (m->fd >= 0)
                close(m->fd);
#endif

        mmap_zero(m);
}