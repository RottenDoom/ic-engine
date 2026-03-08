#include "core/allocators.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

namespace ic
{
static heap_header_t* g_heap_head = nullptr;
static uint64_t g_alloc_id        = 0;

void* debug_malloc(size_t size, const char* file, uint32_t line)
{
        size_t total     = sizeof(heap_header_t) + size;
        heap_header_t* h = (heap_header_t*)malloc(total);

        if (!h)
                return nullptr;
        h->size = size;
        h->file = file;
        h->line = line;

        h->id   = ++g_alloc_id;

        h->prev = nullptr;
        h->next = g_heap_head;

        if (g_heap_head)
                g_heap_head->prev = h;
        g_heap_head = h;

        return (void*)(h + 1);
}

void debug_free(void* ptr)
{
        if (!ptr)
                return;

        heap_header_t* h = ((heap_header_t*)ptr) - 1;

        if (h->prev)
                h->prev->next = h->next;
        if (h->next)
                h->next->prev = h->prev;
        if (g_heap_head == h)
                g_heap_head = h->next;

        free(h);
}

void heap_dump_leaks(void)
{
        heap_header_t* h = g_heap_head;

        if (!h)
        {
                IC_CORE_INFO("No heap leaks detected");
                return;
        }

        IC_CORE_ERROR("Heap leaks detected:");

        while (h)
        {
                IC_CORE_ERROR("  Leak ID={} Size={} bytes at {}:{}", h->id, h->size, h->file, h->line);
                h = h->next;
        }
}

static inline uintptr_t align_forward(uintptr_t ptr, size_t alignment)
{
        return (ptr + (alignment - 1)) & ~(alignment - 1);
}

void bump_allocator_init(bump_allocator_t* bump, void* memory, size_t size)
{
        if (!bump || !memory || size == 0)
                return;

        bump->memory   = (uint8_t*)memory;
        bump->capacity = size;
        bump->offset   = 0;

#if defined(_DEBUG)
        bump->allocation_count = 0;
        bump->generations      = 0;
        bump->high_water_mark  = 0;
#endif
}

void* bump_alloc_tagged(bump_allocator_t* bump, size_t size, size_t alignment, memory_tag tag)
{
        if (size == 0 || alignment == 0)
                return nullptr;

        uintptr_t base          = (uintptr_t)bump->memory;
        uintptr_t current       = base + bump->offset;

        memory_header_t* header = (memory_header_t*)current;

        uintptr_t user_start    = current + sizeof(memory_header_t);
        uintptr_t user_addr     = align_forward(user_start, alignment);
        size_t padding          = (size_t)(user_addr - user_start);
        uintptr_t end_addr      = user_addr + size;

        if (end_addr > base + bump->capacity)
                return nullptr;

        header->size    = size;
        header->tag     = tag;
        header->padding = padding;

        bump->offset    = (size_t)(end_addr - base);

        return (void*)user_addr;
}

void bump_allocator_clear(bump_allocator_t* bump)
{
#if defined(_DEBUG)
        // set the memory to zero since we cleared the memory
        bump->generations++;
        memset(bump->memory, 0, bump->capacity);
#endif

        bump->offset = 0;
}

bump_mark_t bump_mark_push(bump_allocator_t* bump)
{
        bump_mark_t mark;
        mark.offset = bump->offset;

#if defined(_DEBUG)
        mark.generations = bump->generations;
#endif

        return mark;
}

void bump_mark_pop(bump_allocator_t* bump, bump_mark_t mark)
{
#if defined(_DEBUG)
        if (mark.generations != bump->generations)
        {
                IC_CORE_ERROR("Invalid bump mark (generation mismatch)");
                return;
        }

        if (mark.offset > bump->offset)
        {
                IC_CORE_ERROR("Invalid bump mark (offset corruption)");
                return;
        }

        memset(bump->memory + mark.offset, 0xDD, bump->offset - mark.offset);
#endif

        bump->offset = mark.offset;
}

#if defined(_DEBUG)

/** Note do not call this function after clearing the allocator */
void dump_allocations(const bump_allocator_t* bump)
{
        size_t offset = 0;

        IC_CORE_INFO("Allocator Dump:");

        while (offset < bump->offset)
        {
                if (offset + sizeof(memory_header_t) > bump->offset)
                {
                        IC_CORE_ERROR("    Truncated header at offset {}", offset);
                        return;
                }

                const memory_header_t* h = (const memory_header_t*)(bump->memory + offset);

                /** Check if memory is corrupted */
                if (h->canary != IC_CANARY)
                {
                        IC_CORE_ERROR("    Canary corrupted at offset {}", offset);
                        return;
                }

                size_t block_size = sizeof(memory_header_t) + h->size;

                if (offset + block_size > bump->offset)
                {
                        IC_CORE_ERROR("    Allocation overruns arena at offset {}", offset);
                        return;
                }

                IC_CORE_INFO("    ID: {} Size: {} Tag: {} Line: {}, File: {}", h->id, h->size, h->tag, h->line, h->file);

                /** TODO: Block sizes and alignment */
                offset += sizeof(memory_header_t) + h->padding + h->size;
        }

        IC_CORE_INFO("    High-water mark: {} bytes", bump->high_water_mark);
}

void* debug_bump_alloc_tagged(
    bump_allocator_t* bump, size_t size, size_t alignment, memory_tag tag, const char* file, uint32_t line)
{
        if (size == 0 || alignment == 0)
                return nullptr;

        uintptr_t base          = (uintptr_t)bump->memory;
        uintptr_t current       = base + bump->offset;

        memory_header_t* header = (memory_header_t*)current;

        uintptr_t user_start    = current + sizeof(memory_header_t);
        uintptr_t user_addr     = align_forward(user_start, alignment);
        size_t padding          = (size_t)(user_addr - user_start);
        uintptr_t end_addr      = user_addr + size;

        if (end_addr > base + bump->capacity)
                return nullptr;

        header->size    = size;
        header->tag     = tag;
        header->id      = ++bump->allocation_count;
        header->file    = file;
        header->line    = line;
        header->padding = padding;
        header->canary  = IC_CANARY;

        bump->offset    = (size_t)(end_addr - base);
        if (bump->offset > bump->high_water_mark)
                bump->high_water_mark = bump->offset;

        memset((void*)user_addr, 0xCD, size);
        printf("[header: %d, data: %d, size: %zu offset: %d]\n", header, user_addr, size, bump->offset);
        return (void*)user_addr;
}

#endif

}  // namespace ic
