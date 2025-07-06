# setup of the cmake engine directories
set(IC_SOURCES
    src/defines.h
    src/entry.h

    src/core/application.h
    src/core/application.cpp
    src/core/window.h
    src/core/logger.h
    src/core/logger.cpp
    src/core/input.h
    src/core/mousecodes.h
    src/core/keycodes.h
    src/core/events/event.h
    src/core/events/application_event.h
    src/core/events/key_event.h
    src/core/events/mouse_event.h

    src/platform/Win32/window_win32.h
    src/platform/Win32/window_win32.cpp
    src/platform/Win32/input_win32.cpp
)

set(OBJ_DIR ${PROJECT_BINARY_DIR}/obj)
get_subdirectories(SUBDIRS ${CMAKE_CURRENT_LIST_DIR})

# build library (create target)
add_library(engine SHARED ${IC_SOURCES})
set_target_properties(engine PROPERTIES OUTPUT_NAME "engine")

# Find and link Vulkan
find_package(Vulkan REQUIRED)

# Set include directories BEFORE linking libraries
target_include_directories(engine
    PUBLIC
        ${CMAKE_SOURCE_DIR}/engine/src
    PRIVATE
        $ENV{VULKAN_SDK}/Include
)

# Link libraries AFTER setting include directories
target_link_libraries(engine 
    PUBLIC
        spdlog::spdlog
    PRIVATE
        Vulkan::Vulkan
        glfw
)

# Set compiler flags and definitions
target_compile_options(engine PRIVATE
    $<$<CONFIG:Debug>:-g>
    $<$<CONFIG:Release>:-O2>
)

target_link_options(engine PRIVATE
    $<$<CONFIG:Debug>:-g>
    $<$<CONFIG:Release>:-s>  # Strip symbols
)

# Define macros for Debug mode
target_compile_definitions(engine PRIVATE
    $<$<CONFIG:Debug>:_DEBUG IC_ENABLE_ASSERTS>
    $<$<CONFIG:Release>:NDEBUG>
)

set_target_properties(engine PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/bin/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
)
