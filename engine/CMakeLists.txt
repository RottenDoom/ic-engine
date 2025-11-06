set(OBJ_DIR ${PROJECT_BINARY_DIR}/obj)
get_subdirectories(SUBDIRS ${CMAKE_CURRENT_LIST_DIR})

find_package(Vulkan REQUIRED)

# build library (create target)
add_subdirectory(src/core)
add_subdirectory(src/renderer)
add_subdirectory(src/renderer/vulkan)

# Main engine shared library
add_library(engine SHARED
	$<TARGET_OBJECTS:engine-core>
	$<TARGET_OBJECTS:engine-renderer>
	$<TARGET_OBJECTS:engine-vulkan>
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



set_target_properties(engine PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/bin/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
)