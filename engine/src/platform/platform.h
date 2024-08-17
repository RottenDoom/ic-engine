#pragma once

#include "defines.h"

class platform 
{
private:
    const char* application_name;
    i32 x;
    i32 y;
    i32 width;
    i32 height;

    
public:
    typedef struct platform_state {
        void* internal_state;
    } platform_state;

    void* platform_allocate(u64 size, b8 aligned);
    void platform_free(void* block, b8 aligned);
    void* platform_zero_memory(void* block, u64 size);
    void* platform_copy_memory(void* dest, const void* source, u64 size);
    void* platform_set_memory(void* dest, i32 value, u64 size);

    void platform_console_write(const char* message, u8 colour);
    void platform_console_write_error(const char* message, u8 colour);

    f64 platform_get_absolute_time();

    void platform_sleep(u64 ms);

    // default constructor
    platform();

    platform(
        platform::platform_state state,
        const char* application_name,
        i32 x,
        i32 y,
        i32 width,
        i32 height);
    ~platform();
};

IC_API b8 platform_startup(
    platform::platform_state* plat_state,
    const char* application_name,
    i32 x,
    i32 y,
    i32 width,
    i32 height);

IC_API void platform_shutdown(platform::platform_state* plat_state);

IC_API b8 platform_pump_messages(platform::platform_state* plat_state);