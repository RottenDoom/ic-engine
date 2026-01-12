#ifndef ALLOCATORS_H
#define ALLOCATORS_H

#include "../defines.h"
#include <stddef.h>
#include <stdint.h>

#define IC_CANARY 0xDEADC0DE

namespace ic
{
#ifdef __cplusplus
extern "C"
{
#endif
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

        /** @brief Allocator function pointer to allocate a allocator state */
        typedef void* (*ic_alloc_fn)(struct Allocator* allocator, size_t size, size_t alignment, memory_tag tag);
        /** @brief Allocator free function pointer that frees the memory from allocator given to a pointer */
        typedef void (*ic_free_fn)(struct Allocator* allocator, void* ptr);
        /** @brief Destroy function pointer to destroy the allocator of a function */
        typedef void (*ic_destroy_fn)(struct Allocator* allocator);

        /** @brief Optional Dump function for debug */
        typedef void (*ic_dump)(struct Allocator* state);

        /** @interface */
        /** Generic Allocator type */
        typedef struct Allocator
        {
                void* state;
                ic_alloc_fn alloc;
                ic_free_fn free;
                ic_destroy_fn destroy;
                ic_dump dump;
                uint32_t flags;
        } allocator_t;

        void* ic_allocate(allocator_t* allocator, size_t size, size_t alignment, memory_tag tag = IC_TAG_UNKNOWN);
        void ic_free(allocator_t* allocator, void* ptr);
        void ic_allocator_destroy(allocator_t* allocator);
        /** end inteface */

        /** Memory struct header (24 bytes) */
        typedef struct _MemoryHeader
        {
                size_t size;
                memory_tag tag;
                uint32_t id;

#ifdef _DEBUG
                uint32_t canary;
#endif
        } memory_header_t;

        /** ------------ BUMP ALLOCATOR --------------- */

        // Bump allocator for short lived allocations
        typedef struct BumpAllocator
        {
                uint8_t* memory;
                size_t capacity;
                size_t offset;

#if defined(_DEBUG)
                uint32_t allocation_count;
                uint32_t generations;
                size_t high_water_mark;
#endif
        } bump_allocator_t;

        typedef struct BumpMark
        {
                size_t offset;
#if defined(_DEBUG)
                uint32_t generations;
#endif
        } bump_mark_t;

        void bump_allocator_init(bump_allocator_t* bump, void* memory, size_t size);

        void* bump_alloc_tagged(bump_allocator_t* bump, size_t size, size_t alignment, memory_tag tag);

        void bump_allocator_clear(bump_allocator_t* bump);

        bump_mark_t bump_mark_push(bump_allocator_t* bump);
        void bump_mark_pop(bump_allocator_t* bump, bump_mark_t mark);

#if defined(_DEBUG)
        void dump_allocations(const bump_allocator_t* bump);
        IC_API void test_bump_allocator(void);
#endif

#ifdef __cplusplus
}
#endif

}  // namespace ic
#endif