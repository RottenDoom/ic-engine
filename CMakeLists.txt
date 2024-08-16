cmake_minimum_required(VERSION 3.17)

# Set C and C++ standards
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set gcc as the compiler
include(cmake/cmake-compiler/gcc.cmake)

# modules
list(APPEND CMAKE_MODULE_PATH
    ${PROJECT_SOURCE_DIR}/cmake
    ${PROJECT_SOURCE_DIR}/cmake/cmake-compiler
)

include(cmake/ic_version.cmake)
include(cmake/ic_macros.cmake)

project(ic_engine VERSION ${IC_ENGINE_VERSION} LANGUAGES C CXX)
print(PROJECT_VERSION)

include(cmake/ic_options.cmake)
# include(package)

## main engine library
add_subdirectory(engine)

## sample app
add_subdirectory(testbed)
