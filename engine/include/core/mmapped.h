#ifndef MMAPPED_H
#define MMAPPED_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

        typedef enum
        {
                MMAP_NORMAL,      // good general purpose default
                MMAP_SEQUENTIAL,  // reading top to bottom, OS prefetches ahead
                MMAP_RANDOM,      // jumping around, OS won't bother prefetching
        } MmapHint;

        typedef struct
        {
                void    *data;       // pointer to mapped memory, NULL if not mapped
                uint64_t file_size;  // actual file size in bytes
                size_t   map_size;   // how many bytes are currently mapped
                uint64_t offset;     // current mapping offset (for partial maps)

#if defined(_WIN32) || defined(_WIN64)
                void *file_handle;     // HANDLE
                void *mapping_handle;  // HANDLE from CreateFileMapping
#else
        int fd;
#endif
        } Mmap;

        // open and map the whole file. hint tells OS your access pattern
        // returns true on success, false on failure
        bool mmap_open(Mmap *m, const char *path, MmapHint hint);

        // map only a region. offset MUST be a multiple of the system page size
        // use mmap_page_size() to get it
        bool mmap_open_range(Mmap *m, const char *path, uint64_t offset, size_t size, MmapHint hint);

        // unmap and close
        void mmap_close(Mmap *m);

        // remap to a different region of the same file (for streaming large files)
        // offset MUST be page aligned
        bool mmap_remap(Mmap *m, uint64_t offset, size_t size, MmapHint hint);

        // system page size -> offsets for remap/open_range must be multiples of this
        size_t mmap_page_size(void);

        // convenience: is the mapping valid and ready to read
        bool mmap_valid(const Mmap *m);

#ifdef __cplusplus
}
#endif

#endif