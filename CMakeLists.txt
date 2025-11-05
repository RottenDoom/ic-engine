cmake_minimum_required(VERSION 3.28)

# Set C and C++ standards
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
endif()

# modules
list(APPEND CMAKE_MODULE_PATH
    ${PROJECT_SOURCE_DIR}/cmake
    ${PROJECT_SOURCE_DIR}/cmake/cmake-compiler
)

# Keep this for exceptions
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")

# Set gcc as the compiler
include(cmake/version.cmake)
include(cmake/macros.cmake)

project(ic_engine VERSION ${IC_ENGINE_VERSION} LANGUAGES C CXX)
print(PROJECT_VERSION)

include(cmake/options.cmake)

# Include dependencies
include(cmake/dependencies.cmake)

## main engine library
add_subdirectory(engine)

## sample app
add_subdirectory(test-app)
