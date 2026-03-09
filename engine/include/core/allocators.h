#ifndef ALLOCATORS_H
#define ALLOCATORS_H

#include "../defines.h"
#include <stddef.h>
#include <stdint.h>

#define IC_CANARY 0xDEADC0DE
#ifndef NDEBUG
#define ic_malloc(sz) ic::debug_malloc(sz, __FILE__, __LINE__)
#define ic_free(p) ic::debug_free(p)
#define bump_allocate(bump, size, align, tag) ic::debug_bump_alloc_tagged(bump, size, align, tag, __FILE__, __LINE__)
#else
#define ic_malloc(sz) malloc(sz)
#define ic_free(p) free(p)
#define bump_allocate(bump, size, align, tag) ic::bump_alloc_tagged(bump, size, align, tag)
#endif

namespace ic
{
#ifdef __cplusplus
extern "C"
{
#endif

        /** ---------------- HEAP ALLOCATOR -------------- */
        typedef struct heap_header_t
        {
                size_t      size;
                const char *file;
                uint32_t    line;
                uint32_t    id;

                struct heap_header_t *next;
                struct heap_header_t *prev;
        } heap_header_t;

        void *debug_malloc(size_t size, const char *file, uint32_t line);
        void  debug_free(void *ptr);
        void  heap_dump_leaks(void);

        /** ---------------------------------------------- */

        typedef uint32_t memory_tag;

        enum
        {
                IC_TAG_UNKNOWN = 0,
                IC_TAG_FILESYSTEM,
                IC_TAG_FRAME
        };

        /** Allocator flags for different states of allocator */
        typedef enum ic_allocator_flags
        {
                IC_ALLOC_CAN_FREE    = 1 << 0,
                IC_ALLOC_CAN_REALLOC = 1 << 1,
                IC_ALLOC_THREAD_SAFE = 1 << 2
        } ic_allocator_flags;

        /** Memory struct header (24 bytes) */
        typedef struct _MemoryHeader
        {
                size_t     size;
                memory_tag tag;
                uint32_t   id;
                size_t     padding;

#ifndef NDEBUG
                uint32_t    line;
                const char *file;
                uint32_t    canary;
#endif
        } memory_header_t;

        /** ------------ BUMP ALLOCATOR --------------- */

        // Bump allocator for short lived allocations
        typedef struct BumpAllocator
        {
                uint8_t *memory;
                size_t   capacity;
                size_t   offset;

#ifndef NDEBUG
                uint32_t allocation_count;
                uint32_t generations;
                size_t   high_water_mark;
#endif
        } bump_allocator_t;

        typedef struct BumpMark
        {
                size_t offset;
#ifndef NDEBUG
                uint32_t generations;
#endif
        } bump_mark_t;

        // Initialize the allocator with some memory
        void bump_allocator_init(bump_allocator_t *bump, void *memory, size_t size);

        // Allocate memory with a tag
        void *bump_alloc_tagged(bump_allocator_t *bump, size_t size, size_t alignment, memory_tag tag);

        // Clear memory for the whole allocator
        void bump_allocator_clear(bump_allocator_t *bump);

        // Use push mark to allocate without regiestering
        bump_mark_t bump_mark_push(bump_allocator_t *bump);

        // Pop mark to unmark
        void bump_mark_pop(bump_allocator_t *bump, bump_mark_t mark);

#ifndef NDEBUG

        // Dump all the allocation metadata from the allocator
        void  dump_allocations(const bump_allocator_t *bump);
        void *debug_bump_alloc_tagged(
            bump_allocator_t *bump, size_t size, size_t alignment, memory_tag tag, const char *file, uint32_t line);
#endif

#ifdef __cplusplus
}
#endif

}  // namespace ic
#endif