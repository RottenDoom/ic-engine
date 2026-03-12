cmake_minimum_required(VERSION 3.28)

# Set C and C++ standards
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
endif()

message(STATUS "Project build type: ${CMAKE_BUILD_TYPE}")

# modules
list(APPEND CMAKE_MODULE_PATH
    ${PROJECT_SOURCE_DIR}/cmake
    ${PROJECT_SOURCE_DIR}/cmake/cmake-compiler
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")

# set the compiler and version
include(cmake/version.cmake)
include(cmake/macros.cmake)
include(cmake/options.cmake)

project(ic_engine VERSION ${IC_ENGINE_VERSION} LANGUAGES C CXX)
message(STATUS "Current Project Version: ${PROJECT_VERSION}")

# Try to find Vulkan and shader compiler early so config.h can reflect availability
if (IC_ENGINE_USE_VULKAN)
    message(STATUS "Using Vulkan Backend.")
    find_package(Vulkan QUIET)
    if (NOT Vulkan_FOUND)
        message(FATAL_ERROR "IC_ENGINE_USE_VULKAN=ON but Vulkan was not found")
    endif()

    find_program(GLSLC_EXECUTABLE NAMES glslc)
    if (GLSLC_EXECUTABLE)
        set(IC_ENGINE_USE_GLSLC 1)
    else()
        set(IC_ENGINE_USE_GLSLC 0)
    endif()
else()
    message(STATUS "Using OpenGL Backend")
endif()

option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(IC_BUILD_SAMPLES "Build IC engine samples" ON)

configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/config.h.in
    ${CMAKE_BINARY_DIR}/generated/ic_engine/config.h
)

# Include dependencies
include(cmake/dependencies.cmake)

## main engine library
add_subdirectory(engine)

if (IC_BUILD_SAMPLES)
	message(STATUS "Building IC Engine Samples!")
	add_subdirectory(samples)
endif()

## sample app
add_subdirectory(test-app)
