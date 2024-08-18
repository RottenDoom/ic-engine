#pragma once

#include "defines.h"

#include "logger.h"

#include "platform/platform.h"
#include "core/ic_memory.h"
#include "core/event.h"

struct game;

typedef struct application_config {
    i16 start_position_x;
    i16 start_position_y;

    i16 start_width;
    i16 start_height;

    char* name;
} application_config;

class application 
{
private:
    // static variables only stay inside this file
    b8 initialized = FALSE;
    memory mem;
    event ev;
    Logger log;

    // application state
    game* game_inst;
    b8 is_running;
    b8 is_suspended;
    platform::platform_state state;
    i16 width;
    i16 height;
    f64 last_time;
    
public:
    b8 application_create(struct game* game_inst);
    b8 run();
};