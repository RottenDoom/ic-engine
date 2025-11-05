set(OBJ_DIR ${PROJECT_BINARY_DIR}/obj)
get_subdirectories(SUBDIRS ${CMAKE_CURRENT_LIST_DIR})

find_package(Vulkan REQUIRED)

# build library (create target)
add_subdirectory(src/core)
add_subdirectory(src/renderer)
add_subdirectory(src/renderer/vulkan)

# Main engine shared library
add_library(engine SHARED
    src/entry.h
    src/defines.h
)
set_target_properties(engine PROPERTIES OUTPUT_NAME "engine")

target_include_directories(engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src  # For public API headers
    PRIVATE
        ${IC_INTERNAL_HEADERS}
)

target_link_libraries(engine
    PUBLIC
	spdlog::spdlog
    PRIVATE
    	dependencies
        engine-core
        engine-renderer
        engine-vulkan
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
	$<$<CONFIG:Debug>:_DEBUG;IC_ENABLE_ASSERTS;IC_EXPORT>
    	$<$<CONFIG:Release>:NDEBUG;IC_EXPORT>
)

set_target_properties(engine PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/bin/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
)