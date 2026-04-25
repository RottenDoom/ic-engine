cmake_minimum_required(VERSION 3.28)

# IC Engine
set(CMAKE_C_STANDARD          99)
set(CMAKE_CXX_STANDARD        20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

list(APPEND CMAKE_MODULE_PATH
    ${CMAKE_SOURCE_DIR}/cmake
    ${CMAKE_SOURCE_DIR}/cmake/cmake-compiler
)

include(cmake/version.cmake)
include(cmake/macros.cmake)
include(cmake/options.cmake)

project(ic_engine
    VERSION   ${IC_ENGINE_VERSION}
    LANGUAGES C CXX
)

message(STATUS "ic_engine v${PROJECT_VERSION}  build=${CMAKE_BUILD_TYPE}")

# Backend validation
if(IC_ENGINE_USE_VULKAN)
    message(STATUS "Backend: Vulkan")
    find_package(Vulkan QUIET)
    if(NOT Vulkan_FOUND)
        message(FATAL_ERROR "IC_ENGINE_USE_VULKAN=ON but Vulkan SDK not found")
    endif()
    find_program(GLSLC_EXECUTABLE NAMES glslc)
    if(GLSLC_EXECUTABLE)
        message(STATUS "glslc: ${GLSLC_EXECUTABLE}")
        set(IC_ENGINE_USE_GLSLC 1)
    else()
        message(WARNING "glslc not found — shaders will not be compiled")
        set(IC_ENGINE_USE_GLSLC 0)
    endif()
elseif(IC_ENGINE_USE_OPENGL)
    message(STATUS "Backend: OpenGL")
else()
    message(FATAL_ERROR "No backend selected — set IC_ENGINE_USE_OPENGL=ON or IC_ENGINE_USE_VULKAN=ON in options.cmake")
endif()

# Generated config header
configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/config.h.in
    ${CMAKE_BINARY_DIR}/generated/ic_engine/config.h
)

# Third-party dependencies
include(cmake/dependencies.cmake)

# Engine library
add_subdirectory(engine)

# Samples and test app
option(IC_BUILD_SAMPLES "Build IC engine samples" OFF)

if(IC_BUILD_SAMPLES)
    message(STATUS "Building samples")
    add_subdirectory(samples)
endif()

add_subdirectory(test-app)

add_subdirectory(editor)