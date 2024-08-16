# -------------------------------------------------------
# Setup ic_engine include directories and source files
# -------------------------------------------------------

set(IC_SOURCES
    src/test.cpp

    src/test.h
    src/defines.h
)

set(OBJ_DIR ${PROJECT_BINARY_DIR}/obj)

get_subdirectories(SUBDIRS ${CMAKE_CURRENT_LIST_DIR})

# build library (create target)
add_library(engine SHARED ${IC_SOURCES})
set_target_properties(engine PROPERTIES OUTPUT_NAME "engine")

# target_link_libraries()
# Set compiler flags
target_compile_options(engine PRIVATE -g)

# Find and link Vulkan
find_package(Vulkan REQUIRED)

# Set include directories
target_include_directories(engine PRIVATE ${CMAKE_SOURCE_DIR}/engine/src $ENV{VULKAN_SDK}/Include)

# Add linker search paths
link_directories($ENV{VULKAN_SDK}/Lib ${OBJ_DIR})

# Set linker flags
target_link_options(engine PRIVATE -g -shared)
target_link_libraries(engine PRIVATE user32.lib Vulkan::Vulkan)

# Set preprocessor definitions
target_compile_definitions(engine PRIVATE _DEBUG IC_EXPORT _CRT_SECURE_NO_WARNINGS)
